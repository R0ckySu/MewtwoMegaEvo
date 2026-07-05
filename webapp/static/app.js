const { createApp } = Vue;

// ---- Reusable schema-driven field input ---------------------------------
const FieldInput = {
  props: ['field', 'modelValue', 'onPeek'],
  emits: ['update:modelValue'],
  computed: {
    listText() { return Array.isArray(this.modelValue) ? this.modelValue.join('\n') : ''; }
  },
  methods: {
    emit(v) { this.$emit('update:modelValue', v); },
    onList(e) {
      const parts = e.target.value.split(/[\n,]+/).map(s => s.trim()).filter(Boolean);
      this.emit(parts);
    }
  },
  template: `
  <div class="field">
    <label>{{ field.label }}<span v-if="field.optional" class="muted"> (opt)</span></label>

    <input v-if="field.type==='bool'" type="checkbox"
           :checked="!!modelValue" @change="emit($event.target.checked)"
           style="width:auto;align-self:flex-start">

    <select v-else-if="field.type==='enum'"
            :value="modelValue" @change="emit($event.target.value)">
      <option v-for="o in field.options" :key="o" :value="o">{{ o }}</option>
    </select>

    <textarea v-else-if="field.type==='string_list'" rows="3"
              :value="listText" @input="onList"
              placeholder="one per line"></textarea>

    <div v-else-if="field.type==='file'" class="fileinput">
      <input list="wsFiles" class="mono" :value="modelValue"
             @input="emit($event.target.value)" placeholder="file name">
      <button v-if="modelValue" type="button" class="peek" title="Preview file"
              @click="onPeek && onPeek(modelValue)">⤢</button>
    </div>

    <input v-else :value="modelValue" @input="emit($event.target.value)"
           :inputmode="field.type==='int'||field.type==='float' ? 'decimal' : 'text'">
  </div>`
};

// ---- Chip/tag input for lists of symbols (observables, init states) ------
const SymbolList = {
  props: ['modelValue', 'placeholder', 'onPeek', 'listId'],
  emits: ['update:modelValue'],
  data() { return { draft: '' }; },
  methods: {
    items() { return Array.isArray(this.modelValue) ? this.modelValue : []; },
    add() {
      const parts = this.draft.split(/[\s,]+/).map(s => s.trim()).filter(Boolean);
      if (parts.length) this.$emit('update:modelValue', [...this.items(), ...parts]);
      this.draft = '';
    },
    onKey(e) { if (e.key === 'Enter' || e.key === ',') { e.preventDefault(); this.add(); } },
    remove(i) { const a = [...this.items()]; a.splice(i, 1); this.$emit('update:modelValue', a); }
  },
  template: `
  <div class="symlist">
    <span v-for="(it,i) in (modelValue||[])" :key="i" class="tag mono"
      :class="{clickable: onPeek}" title="Click to preview file"
      @click="onPeek && onPeek(it)">{{ it }}
      <span class="x" @click.stop="remove(i)">×</span></span>
    <input class="mono sym-input" :list="listId || 'wsFiles'" v-model="draft"
      :placeholder="placeholder" @keydown="onKey" @blur="add">
  </div>`
};

// ---- Reusable plot panel (used on the Plot page and the Run tab) ---------
const PlotPanel = {
  props: ['runName', 'api', 'notify', 'live', 'refreshSignal'],
  data() {
    return { meta: null, varName: '', obs: '', init: '', mode: 'line',
      x: '', y: '', fixed: {}, xScale: 'linear', yScale: 'linear',
      data: null, liveInfo: null, saved: [], saveName: '', busy: false };
  },
  watch: {
    runName() { this.reload(); },
    live(nv) { this.reload(); },
    refreshSignal() { if (this.live) this.loadLive(); },
  },
  mounted() { this.reload(); },
  methods: {
    reload() {
      if (!this.runName) return;
      if (this.live) this.loadLive(); else this.loadFull();
    },
    varObj() { return (this.meta?.vars || []).find(v => v.name === this.varName); },
    obsList() { return (this.meta?.observables) || []; },
    initList() {
      // init states that pair with the currently selected observable
      const inits = (this.meta?.vars || []).filter(v => v.obs === this.obs).map(v => v.init);
      return inits.length ? inits : ((this.meta?.init_states) || []);
    },
    syncVar() {
      let v = (this.meta?.vars || []).find(x => x.obs === this.obs && x.init === this.init);
      if (!v) { v = (this.meta?.vars || []).find(x => x.obs === this.obs) || (this.meta?.vars || [])[0]; }
      if (v) { this.varName = v.name; this.obs = v.obs; this.init = v.init; }
    },
    onObsInit() { this.syncVar(); this.onVarOrMode(); },
    fixedDims() {
      const dims = this.varObj()?.dims || [];
      return dims.filter(d => d.size > 1 && d.name !== this.x
        && !(this.mode === 'heatmap' && d.name === this.y));
    },
    dimLabel(dim, idx) { return (dim.is_coord && dim.values) ? dim.values[idx] : idx; },
    async loadFull() {
      this.meta = null; this.data = null;
      try {
        const meta = await this.api('/runs/' + encodeURIComponent(this.runName) + '/plot/meta');
        this.meta = meta;
        if (!meta.vars.length) return;
        this.varName = meta.vars[0].name;
        this.obs = meta.vars[0].obs; this.init = meta.vars[0].init;
        this.resetAxes(); this.updateFull(); this.loadSaved();
      } catch (e) { this.notify(e.detail, true); }
    },
    resetAxes() {
      const dims = this.varObj()?.dims || [];
      const coord = dims.filter(d => d.is_coord && d.size > 1);
      this.x = (coord[0] || dims.find(d => d.size > 1) || dims[0] || {}).name || '';
      if (this.mode === 'heatmap') {
        const yc = dims.find(d => d.name !== this.x && d.size > 1)
          || dims.find(d => d.name !== this.x);
        this.y = yc ? yc.name : this.x;
      }
      const f = {}; dims.forEach(d => { f[d.name] = 0; }); this.fixed = f;
    },
    onVarOrMode() { this.resetAxes(); this.updateFull(); },
    async updateFull() {
      if (!this.varName || !this.x) return;
      const req = { mode: this.mode === 'heatmap' ? 'heatmap' : 'series',
        var: this.varName, x: this.x, fixed: this.fixed };
      if (this.mode === 'heatmap') req.y = this.y;
      this.busy = true;
      try {
        const d = await this.api('/runs/' + encodeURIComponent(this.runName) + '/plot/data',
          { method: 'POST', body: req });
        this.data = d;
        this.$nextTick(() => this.renderFull(d));
      } catch (e) { this.notify(e.detail, true); }
      finally { this.busy = false; }
    },
    renderFull(d) {
      const el = this.$refs.plotDiv;
      if (!el || !window.Plotly || !d) return;
      const layout = { margin: { t: 16, r: 16 },
        xaxis: { title: d.x_label, type: this.xScale },
        yaxis: { title: d.y_label, type: this.yScale },
        paper_bgcolor: 'rgba(0,0,0,0)', plot_bgcolor: 'rgba(0,0,0,0)',
        font: { color: '#dce3f0' } };
      const traces = this.mode === 'heatmap'
        ? [{ type: 'heatmap', x: d.x, y: d.y, z: d.z, colorscale: 'Viridis' }]
        : [{ x: d.x, y: d.y, mode: 'lines+markers', type: 'scatter',
             line: { color: '#4f9cff' } }];
      window.Plotly.react(el, traces, layout, { responsive: true, displaylogo: false });
    },
    rerender() {
      // scale change only needs a re-layout, no refetch
      if (this.live) this.renderLive(this.liveInfo); else this.renderFull(this.data);
    },
    // Default save name includes the sliced coordinates to avoid conflicts.
    defaultSaveName() {
      let n = this.obs + '_' + this.init + '_' + this.mode;
      this.fixedDims().forEach(d => {
        n += '_' + d.name + '=' + this.dimLabel(d, this.fixed[d.name]);
      });
      return n.replace(/[^A-Za-z0-9._=-]/g, '_');
    },
    async loadLive() {
      try {
        const info = await this.api('/runs/' + encodeURIComponent(this.runName) + '/plot/live',
          { method: 'POST', body: { var: this.varName || undefined } });
        this.liveInfo = info;
        if (!this.varName && info.vars && info.vars.length) this.varName = info.vars[0];
        this.$nextTick(() => this.renderLive(info));
      } catch (e) { /* run may not have produced data yet; ignore */ }
    },
    renderLive(info) {
      const el = this.$refs.plotDiv;
      if (!el || !window.Plotly || !info.z || !info.z.length) return;
      const np = info.z.length, nm = info.markers;
      const zT = [];
      for (let m = 0; m < nm; m++) {
        const row = []; for (let p = 0; p < np; p++) row.push(info.z[p][m]); zT.push(row);
      }
      const layout = { margin: { t: 16, r: 16 },
        xaxis: { title: 'param index (done)', type: this.xScale },
        yaxis: { title: 'marker', type: this.yScale }, paper_bgcolor: 'rgba(0,0,0,0)',
        plot_bgcolor: 'rgba(0,0,0,0)', font: { color: '#dce3f0' } };
      window.Plotly.react(el, [{ type: 'heatmap', z: zT, colorscale: 'Viridis' }],
        layout, { responsive: true, displaylogo: false });
    },
    async loadSaved() {
      try { this.saved = (await this.api('/runs/' + encodeURIComponent(this.runName)
        + '/plots')).plots; } catch (e) { this.saved = []; }
    },
    async savePlot() {
      const el = this.$refs.plotDiv;
      if (!el || !window.Plotly) return;
      const base = (this.saveName
        ? this.saveName.replace(/[^A-Za-z0-9._=-]/g, '_')
        : this.defaultSaveName());
      try {
        const url = await window.Plotly.toImage(el, { format: 'png', width: 1000, height: 600 });
        await this.api('/runs/' + encodeURIComponent(this.runName) + '/plot/save',
          { method: 'POST', body: { filename: base, png: url } });
        this.saveName = ''; this.notify('Plot saved'); this.loadSaved();
      } catch (e) { this.notify(e.detail || 'Save failed', true); }
    },
    plotUrl(f) {
      return '/api/runs/' + encodeURIComponent(this.runName) + '/plots/' + encodeURIComponent(f);
    },
  },
  template: `
  <div>
    <!-- LIVE (during a run) -->
    <div v-if="live">
      <div class="row" style="gap:12px;margin-bottom:8px;flex-wrap:wrap">
        <label>Observable / init</label>
        <select v-model="varName" @change="loadLive">
          <option v-for="v in (liveInfo && liveInfo.vars || [])" :key="v" :value="v">{{ v }}</option>
        </select>
        <label>X scale</label>
        <select v-model="xScale" @change="rerender">
          <option value="linear">linear</option><option value="log">log</option></select>
        <label>Y scale</label>
        <select v-model="yScale" @change="rerender">
          <option value="linear">linear</option><option value="log">log</option></select>
        <span class="muted">{{ liveInfo ? liveInfo.n_done : 0 }} params done · updating live</span>
      </div>
      <div ref="plotDiv" class="plotbox"></div>
      <p v-if="!liveInfo || !liveInfo.n_done" class="muted">Waiting for first results…</p>
    </div>

    <!-- FULL (completed run) -->
    <div v-else-if="meta && meta.vars.length">
      <div class="row" style="gap:14px;flex-wrap:wrap">
        <div class="field" style="min-width:120px"><label>Observable</label>
          <select v-model="obs" @change="onObsInit">
            <option v-for="o in obsList()" :key="o" :value="o">{{ o }}</option>
          </select></div>
        <div class="field" style="min-width:120px"><label>Init state</label>
          <select v-model="init" @change="onObsInit">
            <option v-for="i in initList()" :key="i" :value="i">{{ i }}</option>
          </select></div>
        <div class="field" style="min-width:110px"><label>Type</label>
          <select v-model="mode" @change="onVarOrMode">
            <option value="line">Line</option><option value="heatmap">Heatmap</option>
          </select></div>
        <div class="field" style="min-width:130px"><label>X axis</label>
          <select v-model="x" @change="updateFull">
            <option v-for="d in varObj().dims" :key="d.name" :value="d.name">{{ d.name }}</option>
          </select></div>
        <div class="field" v-if="mode==='heatmap'" style="min-width:130px"><label>Y axis</label>
          <select v-model="y" @change="updateFull">
            <option v-for="d in varObj().dims.filter(d=>d.name!==x)" :key="d.name"
              :value="d.name">{{ d.name }}</option>
          </select></div>
        <div class="field" style="min-width:90px"><label>X scale</label>
          <select v-model="xScale" @change="rerender">
            <option value="linear">linear</option><option value="log">log</option>
          </select></div>
        <div class="field" style="min-width:90px"><label>Y scale</label>
          <select v-model="yScale" @change="rerender">
            <option value="linear">linear</option><option value="log">log</option>
          </select></div>
      </div>
      <div class="row" v-if="fixedDims().length" style="gap:18px;flex-wrap:wrap;
        border-top:1px solid var(--border);padding-top:10px;margin-top:10px">
        <div v-for="d in fixedDims()" :key="d.name" class="field" style="min-width:190px">
          <label>{{ d.name }} = <span class="mono">{{ dimLabel(d, fixed[d.name]) }}</span>
            <span class="muted">[{{ fixed[d.name] }}/{{ d.size-1 }}]</span></label>
          <input type="range" min="0" :max="d.size-1" v-model.number="fixed[d.name]"
            @change="updateFull">
        </div>
      </div>
      <div ref="plotDiv" class="plotbox"></div>
      <div class="row" style="margin-top:8px;gap:8px">
        <input v-model="saveName" :placeholder="defaultSaveName()" style="width:340px">
        <button @click="savePlot">Save plot</button>
      </div>
      <div v-if="saved.length" style="margin-top:12px">
        <h3>Saved plots</h3>
        <div class="saved-plots">
          <a v-for="s in saved" :key="s.name" :href="plotUrl(s.name)" target="_blank"
            class="saved-thumb"><img :src="plotUrl(s.name)"><span class="mono">{{ s.name }}</span></a>
        </div>
      </div>
    </div>
    <p v-else-if="meta" class="muted">This run has no meas_marker data to plot.</p>
    <p v-else class="muted">Loading result…</p>
  </div>`
};

// ---- Host resource dashboard (per-core CPU + memory) ---------------------
const SystemDashboard = {
  props: ['api'],
  data() { return { sys: null, timer: null }; },
  mounted() { this.poll(); this.timer = setInterval(this.poll, 1500); },
  unmounted() { if (this.timer) clearInterval(this.timer); },
  methods: {
    async poll() { try { this.sys = await this.api('/system'); } catch (e) { /* ignore */ } },
    coreColor(p) { return p >= 85 ? '#ef6a6a' : p >= 55 ? '#e0b341' : '#3ecf8e'; },
    fmtB(b) {
      if (b == null) return '—';
      if (b < 1024) return b + ' B';
      if (b < 1048576) return (b / 1024).toFixed(0) + ' KB';
      if (b < 1073741824) return (b / 1048576).toFixed(0) + ' MB';
      return (b / 1073741824).toFixed(1) + ' GB';
    },
    memLevel() {
      const m = this.sys?.mem?.percent || 0, s = this.sys?.swap?.percent || 0;
      if (m >= 90 || s >= 50) return { t: 'high', c: '#ef6a6a' };
      if (m >= 75 || s >= 10) return { t: 'elevated', c: '#e0b341' };
      return { t: 'ok', c: '#3ecf8e' };
    },
    gaugeStyle(pct, color) {
      const deg = Math.max(0, Math.min(100, pct)) * 3.6;
      return { background: 'conic-gradient(' + color + ' ' + deg + 'deg, var(--panel2) '
        + deg + 'deg)' };
    },
  },
  template: `
  <div class="sysdash">
    <div class="row" style="justify-content:space-between;flex-wrap:wrap;gap:8px">
      <span class="mono" style="font-size:12px">{{ sys ? sys.host.cpu_model : 'host' }}</span>
      <span class="muted" style="font-size:12px" v-if="sys">
        {{ sys.host.cores_logical }} threads<span v-if="sys.host.cores_physical">
          / {{ sys.host.cores_physical }} cores</span>
        <span v-if="sys.cpu.loadavg"> · load {{ sys.cpu.loadavg.join(' ') }}</span>
      </span>
    </div>

    <div v-if="sys" class="gauges">
      <div class="gauge-wrap">
        <div class="gauge" :style="gaugeStyle(sys.cpu.overall||0, coreColor(sys.cpu.overall||0))">
          <div class="gauge-hole"><span class="gauge-val">{{ Math.round(sys.cpu.overall||0) }}<small>%</small></span></div>
        </div>
        <span class="gauge-label">CPU avg</span>
      </div>
      <div class="gauge-wrap">
        <div class="gauge" :style="gaugeStyle(sys.mem.percent, memLevel().c)">
          <div class="gauge-hole"><span class="gauge-val">{{ Math.round(sys.mem.percent) }}<small>%</small></span></div>
        </div>
        <span class="gauge-label">Memory
          <span :style="{color: memLevel().c}">{{ memLevel().t }}</span></span>
      </div>
      <div class="mem-detail">
        <div class="row" style="justify-content:space-between"><label>Memory</label>
          <span class="muted">{{ fmtB(sys.mem.used) }} / {{ fmtB(sys.mem.total) }}
            · {{ fmtB(sys.mem.available) }} free</span></div>
        <div class="membar"><div class="fill" :style="{width: sys.mem.percent + '%',
          background: memLevel().c}"></div></div>
        <div class="row" style="justify-content:space-between;margin-top:8px" v-if="sys.swap.total">
          <label>Swap</label>
          <span class="muted">{{ sys.swap.percent }}% · {{ fmtB(sys.swap.used) }} / {{ fmtB(sys.swap.total) }}</span></div>
        <div class="membar" v-if="sys.swap.total"><div class="fill"
          :style="{width: sys.swap.percent + '%', background: '#8494ad'}"></div></div>
        <p v-if="sys.pressure" class="muted" style="font-size:12px;margin-top:6px">
          Pressure (PSI some): {{ sys.pressure.some_avg10 }}% / 10s ·
          {{ sys.pressure.some_avg60 }}% / 60s</p>
      </div>
    </div>

    <div class="row" style="justify-content:space-between;margin:12px 0 5px">
      <label>CPU per core</label>
      <span class="muted" v-if="sys">{{ sys.cpu.count }} threads</span>
    </div>
    <div class="cores" v-if="sys && sys.cpu.per_core.length">
      <div class="core" v-for="(p,i) in sys.cpu.per_core" :key="i"
        :title="'core ' + i + ': ' + p + '%'">
        <div class="fill" :style="{height: p + '%', background: coreColor(p)}"></div>
      </div>
    </div>
    <p v-else class="muted" style="font-size:12px">measuring…</p>
  </div>`
};

// Clipboard copy that also works over plain-HTTP LAN (non-secure context).
function copyText(text) {
  if (navigator.clipboard && window.isSecureContext) {
    return navigator.clipboard.writeText(text);
  }
  return new Promise((resolve, reject) => {
    const ta = document.createElement('textarea');
    ta.value = text; ta.style.position = 'fixed'; ta.style.opacity = '0';
    document.body.appendChild(ta); ta.focus(); ta.select();
    let ok = false;
    try { ok = document.execCommand('copy'); } catch (e) { ok = false; }
    document.body.removeChild(ta);
    ok ? resolve() : reject(new Error('copy failed'));
  });
}

// ---- Global job queue panel ----------------------------------------------
function fmtDur(s) {
  if (s == null) return '—';
  s = Math.round(s);
  if (s < 60) return s + 's';
  if (s < 3600) return Math.floor(s / 60) + 'm ' + (s % 60) + 's';
  return Math.floor(s / 3600) + 'h ' + Math.floor((s % 3600) / 60) + 'm';
}
const QueuePanel = {
  props: ['api', 'me'],
  data() { return { q: null, timer: null }; },
  mounted() { this.poll(); this.timer = setInterval(this.poll, 2500); },
  unmounted() { if (this.timer) clearInterval(this.timer); },
  methods: {
    async poll() { try { this.q = await this.api('/queue'); } catch (e) { /* ignore */ } },
    fmtDur,
  },
  template: `
  <div v-if="q">
    <div v-if="q.running" class="qrow">
      <span class="status-badge s-running">running</span>
      <span class="mono">{{ q.running.task_name }}</span>
      <span class="muted">· {{ q.running.user }}{{ q.running.user===me ? ' (you)' : '' }}</span>
      <span class="muted" v-if="q.running.total">· {{ q.running.progress }}/{{ q.running.total }}</span>
      <span class="spacer" style="flex:1"></span>
      <span class="muted" v-if="q.running.eta_seconds!=null">~{{ fmtDur(q.running.eta_seconds) }} left</span>
    </div>
    <div v-for="j in q.queue" :key="j.job_id" class="qrow">
      <span class="chip">#{{ j.position }}</span>
      <span class="mono">{{ j.task_name }}</span>
      <span class="muted">· {{ j.user }}{{ j.user===me ? ' (you)' : '' }}</span>
      <span class="spacer" style="flex:1"></span>
      <span class="muted" v-if="j.eta_seconds!=null">starts in ~{{ fmtDur(j.eta_seconds) }}</span>
      <span class="muted" v-else>estimating…</span>
    </div>
    <p v-if="!q.running && !q.queue.length" class="muted">Idle — no jobs running or queued.</p>
    <p v-if="q.secs_per_unit==null && (q.running || q.queue.length)" class="muted"
      style="font-size:12px">ETA improves once a job completes (learning run speed).</p>
  </div>
  <p v-else class="muted">Loading queue…</p>`
};

createApp({
  components: { FieldInput, SymbolList, PlotPanel, SystemDashboard, QueuePanel },
  data() {
    return {
      user: null,
      auth: { mode: 'login', username: '', password: '', error: '' },
      schema: null,
      tab: 'sim',
      demos: [], loadDemoSel: '',
      sim: null, gate: null, ham: null,
      files: [], currentFile: null, fileContent: '', fileMeta: null, fileDirty: false,
      runId: null, run: null, es: null,
      results: [],
      migrationReport: null,
      peek: null,
      view: 'workspace',   // 'workspace' | 'projects' | 'noise'
      runsList: [],
      noiseList: [],
      noiseForm: { tag: '', mode: 'colored', channels: 16, start_idx: 0,
        time_step: 1e-9, length: 100000, amplitude: 1, alpha: 0.9, noise_expr: '' },
      noiseJob: null, noiseEs: null, cfgModal: null,
      plotRun: '',
      busy: false, toast: null, toastErr: false,
    };
  },
  computed: {
    hamTags() {
      return (this.ham?.hamiltonian_prototype_defs || [])
        .map(h => h.tag).filter(Boolean);
    },
    fileNames() { return this.files.filter(f => !f.missing).map(f => f.name); },
    simScalarFields() {
      return (this.schema?.sim_fields || []).filter(
        f => ['string', 'int', 'float', 'enum'].includes(f.type) && f.name !== 'sequence');
    },
    simBoolFields() {
      return (this.schema?.sim_fields || []).filter(f => f.type === 'bool');
    },
    gateTypes() {
      const f = (this.schema?.gate_fields || []).find(x => x.name === 'type');
      return f ? f.options : ['switch'];
    },
    noiseModeExtra() {
      return (this.schema?.noise_mode_extra || {})[this.noiseForm.mode] || [];
    },
    noiseSizeEstimate() {
      // ~23 bytes/sample per channel + a ~10x-length complex spectrum file
      const len = +this.noiseForm.length || 0, ch = +this.noiseForm.channels || 0;
      return ch * len * 23 + 10 * len * 47;
    },
    progressIndet() {
      return this.run && this.run.status === 'running' && !this.run.total;
    },
  },
  created() {
    // Stable wrappers to hand the child components the root's api/notify.
    this.apiFn = (path, opts) => this.api(path, opts);
    this.notifyFn = (msg, isErr) => this.notify(msg, isErr);
  },
  methods: {
    notify(msg, isErr = false) {
      this.toast = msg; this.toastErr = isErr;
      clearTimeout(this._t);
      this._t = setTimeout(() => { this.toast = null; }, isErr ? 6000 : 2500);
    },
    async api(path, opts = {}) {
      const o = Object.assign({ headers: {} }, opts);
      if (o.body !== undefined && typeof o.body !== 'string') {
        o.headers['Content-Type'] = 'application/json';
        o.body = JSON.stringify(o.body);
      }
      const r = await fetch('/api' + path, o);
      if (r.status === 401) { this.user = null; throw { status: 401, detail: 'Please log in' }; }
      let data = null;
      const ct = r.headers.get('content-type') || '';
      if (ct.includes('application/json')) data = await r.json();
      if (!r.ok) throw { status: r.status, detail: (data && data.detail) || r.statusText };
      return data;
    },

    // ---- auth ----
    async doAuth() {
      this.auth.error = '';
      try {
        const ep = this.auth.mode === 'login' ? '/login' : '/register';
        const res = await this.api(ep, { method: 'POST',
          body: { username: this.auth.username, password: this.auth.password } });
        this.user = res.username;
        this.auth.password = '';
        await this.boot();
      } catch (e) { this.auth.error = e.detail || 'Failed'; }
    },
    async logout() {
      try { await this.api('/logout', { method: 'POST' }); } catch (e) {}
      this.closeEvents(); this.closeNoiseEvents();
      Object.assign(this.$data, { user: null, sim: null, gate: null,
        ham: null, files: [], results: [], run: null, runId: null,
        view: 'workspace', runsList: [], plotRun: '' });
    },

    async boot() {
      this.schema = await this.api('/schema');
      await this.loadDemos();
      await this.loadConfigs();
      await this.loadFiles();
    },

    // ---- config source (demos / reset / migrate) ----
    async loadDemos() { this.demos = (await this.api('/demos')).demos; },
    async loadDemo() {
      const demo = this.loadDemoSel;
      if (!demo) return;
      if (!confirm('Load demo "' + demo + '"? This replaces your current config.')) {
        this.loadDemoSel = ''; return;
      }
      try {
        const res = await this.api('/load_demo', { method: 'POST', body: { demo } });
        this.loadDemoSel = '';
        await this.loadConfigs();
        await this.loadFiles();
        this.migrationReport = res.migration || null;
        this.notify('Loaded demo "' + demo + '"');
      } catch (e) { this.notify(e.detail, true); }
    },
    async resetConfig() {
      if (!confirm('Reset your config to blank? This clears the current config.')) return;
      try {
        await this.api('/config/reset', { method: 'POST' });
        await this.loadConfigs();
        await this.loadFiles();
        this.migrationReport = null;
        this.notify('Config reset');
      } catch (e) { this.notify(e.detail, true); }
    },
    async migrateConfigs() {
      try {
        const rep = await this.api('/migrate', { method: 'POST' });
        this.migrationReport = rep;
        await this.loadConfigs();
        await this.loadFiles();
        this.notify(rep.changed ? ('Migrated ' + rep.changes.length + ' field(s)')
          : 'Configs already up to date');
      } catch (e) { this.notify(e.detail, true); }
    },

    // ---- projects portal ----
    showProjects() { this.view = 'projects'; this.loadRuns(); },
    async loadRuns() {
      try { this.runsList = (await this.api('/runs')).runs; }
      catch (e) { this.notify(e.detail, true); }
    },
    runDownloadUrl(r) {
      return '/api/runs/' + encodeURIComponent(r.name) + '/download';
    },
    fmtDate(t) { return new Date(t * 1000).toLocaleString(); },
    fmtDur,
    async copyRunConfig(r) {
      if (!confirm('Load config from run "' + r.name + '" into your editor? '
        + 'This replaces your current config.')) return;
      try {
        const res = await this.api('/runs/copy', { method: 'POST',
          body: { run_name: r.name } });
        this.view = 'workspace'; this.tab = 'sim';
        await this.loadConfigs();
        await this.loadFiles();
        this.migrationReport = res.migration || null;
        this.notify('Loaded config from "' + r.name + '"');
      } catch (e) { this.notify(e.detail, true); }
    },
    async deleteRun(r) {
      if (!confirm('Delete run "' + r.name + '"? This removes its results '
        + 'and saved config permanently.')) return;
      try {
        await this.api('/runs/' + encodeURIComponent(r.name), { method: 'DELETE' });
        await this.loadRuns();
        this.notify('Run deleted');
      } catch (e) { this.notify(e.detail, true); }
    },

    // ---- data plots (meas_marker) — handled by the <plot-panel> component ----
    openPlot(name) { this.view = 'plot'; this.plotRun = name; },

    // ---- shared noise generation ----
    showNoise() { this.view = 'noise'; this.loadNoise(); },
    async loadNoise() {
      try { this.noiseList = (await this.api('/noise')).noise; }
      catch (e) { this.notify(e.detail, true); }
    },
    async generateNoise() {
      this.closeNoiseEvents();
      this.noiseJob = { status: 'starting', progress: 0, total: null, percent: null, log: [] };
      try {
        const res = await this.api('/noise/generate', { method: 'POST', body: this.noiseForm });
        this.noiseJob.tag = res.tag;
        const es = new EventSource('/api/noise/jobs/' + res.job_id + '/events');
        this.noiseEs = es;
        es.onmessage = (ev) => {
          this.noiseJob = JSON.parse(ev.data);
          if (['done', 'error'].includes(this.noiseJob.status)) {
            this.closeNoiseEvents();
            this.notify(this.noiseJob.status === 'done' ? 'Noise generated' : 'Generation failed',
              this.noiseJob.status === 'error');
            this.loadNoise();
          }
        };
      } catch (e) { this.noiseJob = null; this.notify(e.detail, true); }
    },
    closeNoiseEvents() { if (this.noiseEs) { this.noiseEs.close(); this.noiseEs = null; } },
    async viewNoiseConfig(group) {
      try {
        const cfg = await this.api('/noise/' + encodeURIComponent(group) + '/config');
        this.cfgModal = { title: group, json: JSON.stringify(cfg, null, 2) };
      } catch (e) { this.notify(e.detail, true); }
    },
    async copyNoisePath(n) {
      try { await copyText(n.waveform_path); this.notify('Copied: ' + n.waveform_path); }
      catch (e) { this.notify('Copy failed — path: ' + n.waveform_path, true); }
    },
    async deleteNoise(group) {
      if (!confirm('Delete noise group "' + group + '"? This is shared by all users '
        + 'and removes the cached data permanently.')) return;
      try {
        await this.api('/noise/' + encodeURIComponent(group), { method: 'DELETE' });
        await this.loadNoise();
        this.notify('Noise group deleted');
      } catch (e) { this.notify(e.detail, true); }
    },

    // ---- configs ----
    async loadConfigs() {
      this.sim = await this.api('/config/sim');
      if (!this.sim.sweep_param_info) this.sim.sweep_param_info = [];
      if (!Array.isArray(this.sim.observables)) this.sim.observables = [];
      if (!Array.isArray(this.sim.init_states)) this.sim.init_states = [];
      this.sim.sweep_param_info.forEach(s => {
        if (!('val_file' in s)) s.val_file = '';
        if (!('string_file' in s)) s.string_file = '';
        s._vtype = (s.class !== 'Sequence' && s.string_file) ? 'string' : 'numerical';
      });
      this.gate = await this.api('/config/gate');
      if (!this.gate.gate_defs) this.gate.gate_defs = [];
      this.ham = await this.api('/config/hamiltonian');
      if (!this.ham.hamiltonian_prototype_defs) this.ham.hamiltonian_prototype_defs = [];
    },
    async saveConfig(kind, obj, quiet = false) {
      try {
        await this.api('/config/' + kind, { method: 'PUT', body: obj });
        if (!quiet) this.notify('Saved ' + kind + '_config.json');
        return true;
      } catch (e) { this.notify('Save ' + kind + ': ' + e.detail, true); return false; }
    },
    async saveAll(quiet = false) {
      const a = await this.saveConfig('sim', this.sim, true);
      const b = await this.saveConfig('gate', this.gate, true);
      const c = await this.saveConfig('hamiltonian', this.ham, true);
      const ok = a && b && c;
      if (ok && !quiet) this.notify('All configs saved');
      if (ok) await this.loadFiles();
      return ok;
    },

    // ---- sweep / gate / ham editing ----
    addSweep() { this.sim.sweep_param_info.push(
      { class: 'Gate', tag: '', property: '', val_file: '', string_file: '', _vtype: 'numerical' }); },
    removeSweep(i) { this.sim.sweep_param_info.splice(i, 1); },
    // A row uses a string-valued vector when it's a Sequence sweep, or a
    // Hamiltonian sweep whose value type is set to "string".
    sweepIsString(s) {
      return s.class === 'Sequence' || (s.class === 'Hamiltonian' && s._vtype === 'string');
    },
    onSweepClass(s) {
      if (s.class === 'Sequence') { s._vtype = 'string'; s.property = ''; s.val_file = ''; }
      else if (s.class === 'Gate') { s._vtype = 'numerical'; s.string_file = ''; }
      // Hamiltonian keeps whatever _vtype it had (default numerical)
      if (s._vtype === 'numerical') s.string_file = ''; else s.val_file = '';
    },
    onSweepVType(s, v) {
      s._vtype = v;
      if (v === 'numerical') s.string_file = ''; else s.val_file = '';
    },

    addGate() { this.gate.gate_defs.push(
      { tag: '', type: 'switch', hamiltonians: [], pulse_width: 0, shift_time: 0,
        ext_shaped_sig_path: '' }); },
    removeGate(i) { this.gate.gate_defs.splice(i, 1); },

    hamFields(type) { return (this.schema.hamiltonian_types[type]) || []; },
    // Order the schema fields to match the key order in the actual config object
    // (i.e. as written in the JSON file), appending any schema fields the object
    // doesn't have yet. Keys not in the schema (e.g. stale fields from a former
    // type, or the header-handled 'type') are skipped.
    orderFields(obj, schemaFields) {
      const byName = {};
      schemaFields.forEach(f => { byName[f.name] = f; });
      const out = [], seen = new Set();
      Object.keys(obj).forEach(k => {
        if (byName[k]) { out.push(byName[k]); seen.add(k); }
      });
      schemaFields.forEach(f => { if (!seen.has(f.name)) out.push(f); });
      return out;
    },
    orderedGateFields(g) { return this.orderFields(g, this.schema.gate_fields); },
    orderedHamFields(h) { return this.orderFields(h, this.hamFields(h.type)); },
    // Subtle, muted accent colour per gate/Hamiltonian type.
    typeColor(type) {
      return ({
        switch: '#4f9cff', shaped: '#b083f0', sticky: '#e0b341',
        static: '#7c89a6', static_RF: '#5aa0a0', mw: '#4f9cff',
        mw_RF: '#3aa3c9', awg: '#3ecf8e', noise: '#e08a5a',
      })[type] || '#7c89a6';
    },
    cardStyle(type) { return { borderLeft: '3px solid ' + this.typeColor(type) }; },
    addHam() {
      const f = {};
      this.hamFields('static').forEach(fd => {
        f[fd.name] = fd.type === 'bool' ? false : (fd.type === 'float' || fd.type === 'int' ? 0 : ''); });
      f.type = 'static'; f.enable = true;
      this.ham.hamiltonian_prototype_defs.push(f);
    },
    changeHamType(h, type) {
      h.type = type;
      this.hamFields(type).forEach(fd => {
        if (h[fd.name] === undefined)
          h[fd.name] = fd.type === 'bool' ? false : (fd.type === 'float' || fd.type === 'int' ? 0 : ''); });
    },
    removeHam(i) { this.ham.hamiltonian_prototype_defs.splice(i, 1); },

    // ---- files ----
    async loadFiles() {
      this.files = (await this.api('/files')).files;
    },
    async openFile(f) {
      if (f.missing) { this.notify('Referenced file is missing on disk', true); return; }
      if (this.fileDirty && !confirm('Discard unsaved changes to ' + this.currentFile + '?'))
        return;
      const data = await this.api('/files/' + encodeURIComponent(f.name));
      this.currentFile = f.name; this.fileMeta = data; this.fileContent = data.content;
      this.fileDirty = false;
    },
    async saveFile() {
      try {
        await this.api('/files/' + encodeURIComponent(this.currentFile),
          { method: 'PUT', body: { content: this.fileContent } });
        this.fileDirty = false; this.notify('Saved ' + this.currentFile);
        await this.loadFiles();
      } catch (e) { this.notify(e.detail, true); }
    },
    async newFile() {
      const name = prompt('New file name:');
      if (!name) return;
      try {
        await this.api('/files', { method: 'POST',
          body: { name: name, content: '' } });
        await this.loadFiles();
        await this.openFile({ name: name });
      } catch (e) { this.notify(e.detail, true); }
    },
    async deleteFile(f) {
      if (!confirm('Delete file "' + f.name + '"?')) return;
      try {
        await this.api('/files/' + encodeURIComponent(f.name),
          { method: 'DELETE' });
        if (this.currentFile === f.name) { this.currentFile = null; this.fileContent = ''; }
        await this.loadFiles();
      } catch (e) { this.notify(e.detail, true); }
    },

    // ---- run ----
    async startRun() {
      if (!await this.saveAll(true)) { this.notify('Fix config errors before running', true); return; }
      this.closeEvents();
      this.run = { status: 'starting', progress: 0, total: null, percent: null, log: [] };
      try {
        const res = await this.api('/run', { method: 'POST' });
        this.runId = res.run_id;
        this.connectEvents(res.run_id);
      } catch (e) { this.run = null; this.notify(e.detail, true); }
    },
    connectEvents(runId) {
      const url = '/api/run/' + runId + '/events';
      this.es = new EventSource(url);
      this.es.onmessage = (ev) => {
        this.run = JSON.parse(ev.data);
        if (['done', 'error', 'stopped'].includes(this.run.status)) {
          this.closeEvents();
          if (this.run.status === 'done') { this.notify('Simulation complete'); this.loadResults(); }
          else if (this.run.status === 'error') this.notify('Simulation failed (see log)', true);
        }
      };
      this.es.onerror = () => { /* stream ends on completion; ignore */ };
    },
    closeEvents() { if (this.es) { this.es.close(); this.es = null; } },
    async stopRun() {
      if (!this.runId) return;
      try { await this.api('/run/' + this.runId + '/stop',
        { method: 'POST' }); } catch (e) { this.notify(e.detail, true); }
    },

    // ---- matrix/vector peek popup ----
    async peekFile(name) {
      if (!name) return;
      try {
        const d = await this.api('/files/' + encodeURIComponent(name));
        const rows = d.content.replace(/\r/g, '').split('\n').filter(l => l.trim().length);
        const grid = rows.map(r => r.split(',').map(c => c.trim()));
        const cols = grid.reduce((m, r) => Math.max(m, r.length), 0);
        this.peek = { name: d.name, size: d.size, truncated: d.truncated, content: d.content,
          grid, rows: grid.length, cols, asTable: cols > 1 && grid.length <= 200 };
      } catch (e) {
        this.notify(e.status === 404
          ? ('"' + name + '" is not a file here (built-in symbol, or file missing)')
          : e.detail, true);
      }
    },
    closePeek() { this.peek = null; },
    peekToEditor() {
      const n = this.peek.name; this.closePeek(); this.tab = 'files'; this.openFile({ name: n });
    },

    // ---- results (runs are listed directly under the user) ----
    async loadResults() {
      this.results = (await this.api('/runs')).runs;
    },
    downloadUrl(name) {
      return '/api/runs/' + encodeURIComponent(name) + '/download';
    },
    fmtSize(b) {
      if (b < 1024) return b + ' B';
      if (b < 1048576) return (b / 1024).toFixed(1) + ' KB';
      return (b / 1048576).toFixed(1) + ' MB';
    },
  },
  async mounted() {
    try { const me = await this.api('/me'); this.user = me.username; await this.boot(); }
    catch (e) { /* not logged in */ }
  },
  template: `
  <div v-if="!user" class="login">
    <div class="card">
      <h2>MewtwoMegaEvo</h2>
      <div class="field"><label>Username</label>
        <input v-model="auth.username" @keyup.enter="doAuth"></div>
      <div class="field"><label>Password</label>
        <input type="password" v-model="auth.password" @keyup.enter="doAuth"></div>
      <div class="err" v-if="auth.error">{{ auth.error }}</div>
      <div class="row" style="margin-top:12px">
        <button @click="doAuth">{{ auth.mode === 'login' ? 'Log in' : 'Register' }}</button>
        <button class="ghost" @click="auth.mode = auth.mode==='login'?'register':'login'; auth.error=''">
          {{ auth.mode === 'login' ? 'Need an account?' : 'Have an account?' }}
        </button>
      </div>
    </div>
  </div>

  <div v-else>
    <datalist id="wsFiles">
      <option v-for="n in fileNames" :key="n" :value="n"></option>
    </datalist>
    <datalist id="hamTags">
      <option v-for="t in hamTags" :key="t" :value="t"></option>
    </datalist>

    <div class="topbar">
      <h1>MewtwoMegaEvo</h1>
      <div class="nav">
        <button class="ghost" :class="{active: view==='workspace'}"
          @click="view='workspace'">Editor</button>
        <button class="ghost" :class="{active: view==='projects'}"
          @click="showProjects">Projects</button>
        <button class="ghost" :class="{active: view==='noise'}"
          @click="showNoise">Noise</button>
      </div>
      <div class="row" v-show="view==='workspace'">
        <select :value="loadDemoSel" @change="loadDemoSel=$event.target.value; loadDemo()"
          title="Replace current config with a demo template">
          <option value="">Load demo…</option>
          <option v-for="d in demos" :key="d" :value="d">{{ d }}</option>
        </select>
        <button class="ghost" @click="resetConfig">Reset</button>
      </div>
      <div class="spacer"></div>
      <span class="muted">{{ user }}</span>
      <button class="ghost" @click="logout">Log out</button>
    </div>

    <div class="tabs" v-if="view==='workspace'">
      <button v-for="t in ['sim','gates','hamiltonians','files','run']" :key="t"
        :class="{active: tab===t}" @click="tab=t">{{ t }}</button>
    </div>

    <!-- PROJECTS PORTAL -->
    <div class="wrap" v-if="view==='projects'">
      <div class="card">
        <div class="row" style="justify-content:space-between">
          <h2 style="margin:0">Projects — previous runs</h2>
          <button class="ghost" @click="loadRuns">Refresh</button>
        </div>
        <p class="muted">Your previous runs (each is a <span class="mono">&lt;task&gt;&lt;timestamp&gt;</span>
          folder). "Load config" replaces your editor config with the one this run used.</p>
        <table class="sweep" v-if="runsList.length">
          <thead><tr><th>Run</th><th>Finished</th><th>Size</th>
            <th>Results</th><th></th></tr></thead>
          <tbody>
            <tr v-for="r in runsList" :key="r.name">
              <td class="mono">{{ r.name }}</td>
              <td class="muted">{{ fmtDate(r.mtime) }}</td>
              <td class="muted">{{ fmtSize(r.size) }}</td>
              <td class="muted">{{ r.h5_files.join(', ') || '—' }}</td>
              <td>
                <div class="row">
                  <button @click="openPlot(r.name)" :disabled="!r.h5_files.length"
                    :title="r.h5_files.length ? 'Browse meas_marker plots'
                      : 'No result data'">Plot</button>
                  <a :href="runDownloadUrl(r)"><button class="ghost">Download</button></a>
                  <button class="ghost" @click="copyRunConfig(r)" :disabled="!r.has_config"
                    :title="r.has_config ? 'Load this run config into the editor'
                      : 'No saved config for this run'">Load config</button>
                  <button class="danger" @click="deleteRun(r)">Delete</button>
                </div>
              </td>
            </tr>
          </tbody>
        </table>
        <p v-else class="muted">No runs yet. Launch a simulation from the Editor's Run tab.</p>
      </div>
    </div>

    <!-- NOISE (shared) -->
    <div class="wrap wide" v-else-if="view==='noise'">
      <div class="noise-split">
        <!-- LEFT: cached noise on the server -->
        <div class="card">
          <div class="row" style="justify-content:space-between">
            <h2 style="margin:0">Cached noise groups</h2>
            <button class="ghost" @click="loadNoise">Refresh</button>
          </div>
          <p class="muted">Shared by all users. <strong>Click a group's name</strong> to copy its
            <span class="mono">waveform_path</span> (for a noise Hamiltonian); click elsewhere in the
            row to view the config it was generated with.</p>
          <table class="sweep" v-if="noiseList.length">
            <thead><tr><th>Group</th><th>time_step</th><th>channels</th><th>length</th>
              <th>mode</th><th>size</th><th></th></tr></thead>
            <tbody>
              <tr v-for="n in noiseList" :key="n.group" class="clickrow"
                @click="viewNoiseConfig(n.group)">
                <td class="mono">
                  <span class="copyname" @click.stop="copyNoisePath(n)"
                    :title="'Click to copy: ' + n.waveform_path">{{ n.group }}</span>
                  <span v-if="n.generating" class="chip">generating…</span></td>
                <td class="muted">{{ n.time_step != null ? n.time_step : '—' }}</td>
                <td class="muted">{{ n.channels != null ? n.channels : '—' }}</td>
                <td class="muted">{{ n.length != null ? n.length : '—' }}</td>
                <td class="muted">{{ n.mode || '—' }}</td>
                <td class="muted">{{ fmtSize(n.size) }}</td>
                <td @click.stop>
                  <button class="danger" @click="deleteNoise(n.group)"
                    :disabled="n.generating">Delete</button>
                </td>
              </tr>
            </tbody>
          </table>
          <p v-else class="muted">No noise groups cached yet.</p>
        </div>

        <!-- RIGHT: generate new noise -->
        <div class="card">
          <h2>Generate noise</h2>
          <p class="muted">Runs the NoiseGen binary; data is written to the shared
            <span class="mono">NoiseData</span> store. Generation can be large and slow.</p>
          <div class="cardfields">
            <field-input v-for="f in (schema.noise_fields||[])" :key="f.name"
              :field="f" v-model="noiseForm[f.name]"></field-input>
            <field-input v-for="f in noiseModeExtra" :key="f.name"
              :field="f" v-model="noiseForm[f.name]"></field-input>
          </div>
          <div class="row" style="margin-top:12px;justify-content:space-between">
            <span class="muted">Est. size ≈ {{ fmtSize(noiseSizeEstimate) }}</span>
            <button @click="generateNoise"
              :disabled="noiseJob && noiseJob.status==='running' || !noiseForm.tag">Generate</button>
          </div>
          <div v-if="noiseJob" style="margin-top:12px">
            <div class="row" style="justify-content:space-between;margin-bottom:6px">
              <span class="status-badge" :class="'s-'+noiseJob.status">{{ noiseJob.status }}</span>
              <span class="muted" v-if="noiseJob.total">{{ noiseJob.progress }} / {{ noiseJob.total }} ch</span>
            </div>
            <div class="progress" :class="{indet: noiseJob.status==='running' && !noiseJob.total}">
              <div class="bar" :style="{width: (noiseJob.percent||0)+'%'}"></div>
              <div class="label">{{ noiseJob.percent!=null ? noiseJob.percent+'%' : '' }}</div>
            </div>
            <div class="logbox mono" style="height:110px;margin-top:8px">{{ (noiseJob.log||[]).join('\\n') }}</div>
          </div>
        </div>
      </div>
    </div>

    <!-- PLOTS (meas_marker) -->
    <div class="wrap wide" v-else-if="view==='plot'">
      <div class="card">
        <div class="row" style="justify-content:space-between;margin-bottom:8px">
          <h2 style="margin:0">Plot — <span class="mono">{{ plotRun }}</span></h2>
          <button class="ghost" @click="showProjects">← Back to Projects</button>
        </div>
        <plot-panel :key="plotRun" :run-name="plotRun" :api="apiFn" :notify="notifyFn"
          :live="false"></plot-panel>
      </div>
    </div>

    <div class="wrap" v-else>
      <template v-if="sim && gate && ham">
        <!-- migration warnings banner -->
        <div v-if="migrationReport && migrationReport.warnings && migrationReport.warnings.length"
             class="card" style="border-left:3px solid var(--warn)">
          <strong class="warn">⚠ Remaining blockers after migration</strong>
          <ul style="margin:8px 0 0 18px">
            <li v-for="(w,i) in migrationReport.warnings" :key="i" class="muted">{{ w }}</li>
          </ul>
        </div>

        <!-- SIM -->
        <div v-show="tab==='sim'">
          <div class="card">
            <div class="row" style="justify-content:space-between">
              <h2 style="margin:0">Simulation config</h2>
              <button class="ghost" @click="migrateConfigs"
                title="Fill any fields the current binary requires (for legacy configs)">
                Migrate legacy configs</button>
            </div>

            <h3>General</h3>
            <div class="grid">
              <field-input v-for="f in simScalarFields" :key="f.name"
                :field="f" v-model="sim[f.name]" :on-peek="peekFile"></field-input>
            </div>

            <h3 style="margin-top:16px">Recording &amp; mode</h3>
            <div class="checks">
              <label class="check" v-for="f in simBoolFields" :key="f.name">
                <input type="checkbox" :checked="!!sim[f.name]"
                  @change="sim[f.name]=$event.target.checked"> {{ f.label }}
              </label>
            </div>

            <h3 style="margin-top:16px">States &amp; observables</h3>
            <div class="grid">
              <div class="field">
                <label>Observables <span class="muted">(operator symbols)</span></label>
                <symbol-list v-model="sim.observables" placeholder="add symbol…"
                  :on-peek="peekFile"></symbol-list>
              </div>
              <div class="field">
                <label>Initial states <span class="muted">(density-matrix symbols)</span></label>
                <symbol-list v-model="sim.init_states" placeholder="add symbol…"
                  :on-peek="peekFile"></symbol-list>
              </div>
            </div>

            <h3 style="margin-top:16px">Sequence</h3>
            <div class="field">
              <input class="mono" v-model="sim.sequence"
                placeholder="e.g. $S-M   or   [X]^2-F-M">
            </div>
          </div>

          <div class="card">
            <div class="row" style="justify-content:space-between">
              <h2 style="margin:0">Sweep parameters</h2>
              <button @click="addSweep">+ Add sweep</button>
            </div>
            <p class="muted">Each entry sweeps a property over the values in a file. The
              <strong>value type</strong> sets how the vector is decoded: numerical (val_file)
              or string (string_file). Sequence sweeps are always string; Gate sweeps are always
              numerical; Hamiltonian sweeps may be either.</p>
            <table class="sweep" v-if="sim.sweep_param_info.length">
              <thead><tr><th>Class</th><th>Tag</th><th>Property</th>
                <th>Parameter file</th><th></th></tr></thead>
              <tbody>
                <tr v-for="(s,i) in sim.sweep_param_info" :key="i">
                  <td style="width:120px">
                    <select v-model="s.class" @change="onSweepClass(s)">
                      <option v-for="c in schema.sweep_classes" :key="c" :value="c">{{ c }}</option>
                    </select>
                  </td>
                  <td><input v-model="s.tag" placeholder="tag"></td>
                  <td><input v-model="s.property" :disabled="s.class==='Sequence'"
                        :placeholder="s.class==='Sequence' ? 'n/a' : 'e.g. amplitude'"></td>
                  <td>
                    <div class="filecell">
                      <select class="vtype" :value="sweepIsString(s) ? 'string' : 'numerical'"
                        :disabled="s.class!=='Hamiltonian'"
                        @change="onSweepVType(s,$event.target.value)"
                        title="How the file is decoded">
                        <option value="numerical">num</option>
                        <option value="string">str</option>
                      </select>
                      <input v-if="sweepIsString(s)" v-model="s.string_file" list="wsFiles"
                        class="mono" placeholder="string file">
                      <input v-else v-model="s.val_file" list="wsFiles" class="mono"
                        placeholder="numerical file">
                      <button type="button" class="peek"
                        v-if="sweepIsString(s) ? s.string_file : s.val_file"
                        @click="peekFile(sweepIsString(s) ? s.string_file : s.val_file)"
                        title="Preview file">⤢</button>
                    </div>
                  </td>
                  <td style="width:34px"><button class="danger" @click="removeSweep(i)">×</button></td>
                </tr>
              </tbody>
            </table>
            <p v-else class="muted">No sweep parameters.</p>
          </div>
          <button @click="saveConfig('sim', sim)">Save sim config</button>
        </div>

        <!-- GATES -->
        <div v-show="tab==='gates'">
          <div class="row" style="justify-content:space-between">
            <h2 style="margin:0">Gates</h2>
            <button @click="addGate">+ Add gate</button>
          </div>
          <div class="masonry">
            <div class="item masonry-card" v-for="(g,i) in gate.gate_defs" :key="i"
              :style="cardStyle(g.type)">
              <div class="item-head">
                <div class="row">
                  <span class="type-dot" :style="{background: typeColor(g.type)}"></span>
                  <strong class="mono">{{ g.tag || '(untitled gate)' }}</strong>
                  <label>type</label>
                  <select v-model="g.type">
                    <option v-for="t in gateTypes" :key="t" :value="t">{{ t }}</option>
                  </select>
                </div>
                <button class="danger" @click="removeGate(i)">Remove</button>
              </div>
              <div class="cardfields">
                <template v-for="f in orderedGateFields(g).filter(f=>f.name!=='type')" :key="f.name">
                  <div v-if="f.type==='hamiltonian_multiselect'" class="field">
                    <label>{{ f.label }} <span class="muted">(defined Hamiltonians)</span></label>
                    <symbol-list v-model="g.hamiltonians" list-id="hamTags"
                      placeholder="add Hamiltonian…"></symbol-list>
                  </div>
                  <field-input v-else :field="f" v-model="g[f.name]"
                    :on-peek="peekFile"></field-input>
                </template>
              </div>
            </div>
          </div>
          <p v-if="!gate.gate_defs.length" class="muted">No gates defined.</p>
          <button @click="saveConfig('gate', gate)">Save gate config</button>
        </div>

        <!-- HAMILTONIANS -->
        <div v-show="tab==='hamiltonians'">
          <div class="row" style="justify-content:space-between">
            <h2 style="margin:0">Hamiltonians</h2>
            <button @click="addHam">+ Add Hamiltonian</button>
          </div>
          <div class="masonry">
            <div class="item masonry-card" v-for="(h,i) in ham.hamiltonian_prototype_defs" :key="i"
              :style="cardStyle(h.type)">
              <div class="item-head">
                <div class="row">
                  <span class="type-dot" :style="{background: typeColor(h.type)}"></span>
                  <strong class="mono">{{ h.tag || '(untitled)' }}</strong>
                  <label>type</label>
                  <select :value="h.type" @change="changeHamType(h,$event.target.value)">
                    <option v-for="t in Object.keys(schema.hamiltonian_types)" :key="t" :value="t">
                      {{ t }}</option>
                  </select>
                </div>
                <button class="danger" @click="removeHam(i)">Remove</button>
              </div>
              <div class="cardfields">
                <field-input v-for="f in orderedHamFields(h)" :key="f.name"
                  :field="f" v-model="h[f.name]" :on-peek="peekFile"></field-input>
              </div>
            </div>
          </div>
          <p v-if="!ham.hamiltonian_prototype_defs.length" class="muted">No Hamiltonians defined.</p>
          <button @click="saveConfig('hamiltonian', ham)">Save Hamiltonian config</button>
        </div>

        <!-- FILES -->
        <div v-show="tab==='files'">
          <h2>Matrix / vector / parameter files</h2>
          <div class="files-layout">
            <div>
              <div class="row" style="margin-bottom:8px">
                <button @click="newFile">+ New</button>
                <button class="ghost" @click="loadFiles">Refresh</button>
              </div>
              <div class="file-list">
                <div v-for="f in files" :key="f.name" class="f"
                  :class="{active: currentFile===f.name}" @click="openFile(f)">
                  <span class="fn mono">{{ f.name }}</span>
                  <span>
                    <span v-if="f.missing" class="chip missing">missing</span>
                    <span v-else-if="f.referenced" class="chip ref">ref</span>
                  </span>
                </div>
                <div v-if="!files.length" class="f muted">No files.</div>
              </div>
            </div>
            <div class="editor">
              <div v-if="currentFile">
                <div class="row" style="justify-content:space-between;margin-bottom:8px">
                  <span class="mono">{{ currentFile }}
                    <span class="muted">({{ fmtSize(fileMeta.size) }})</span>
                    <span v-if="fileMeta.truncated" class="chip missing">truncated preview</span>
                    <span v-if="fileDirty" class="chip">unsaved</span>
                  </span>
                  <div class="row">
                    <button @click="saveFile" :disabled="fileMeta.truncated">Save</button>
                    <button class="danger" @click="deleteFile({name: currentFile})">Delete</button>
                  </div>
                </div>
                <textarea class="mono" v-model="fileContent" @input="fileDirty=true"
                  spellcheck="false" :readonly="fileMeta.truncated"></textarea>
                <p class="muted" style="margin-top:6px">
                  Matrices: comma-separated per row. Vectors: one value per line.</p>
              </div>
              <p v-else class="muted">Select a file to view or edit.</p>
            </div>
          </div>
        </div>

        <!-- RUN -->
        <div v-show="tab==='run'">
          <div class="card" v-if="tab==='run'">
            <h2>Host resources</h2>
            <system-dashboard :api="apiFn"></system-dashboard>
          </div>

          <div class="card" v-if="tab==='run'">
            <div class="row" style="justify-content:space-between">
              <h2 style="margin:0">Job queue</h2>
              <span class="muted" style="font-size:12px">one job runs at a time, server-wide</span>
            </div>
            <queue-panel :api="apiFn" :me="user"></queue-panel>
          </div>

          <div class="card">
            <div class="row" style="justify-content:space-between">
              <h2 style="margin:0">Run simulation</h2>
              <div class="row">
                <button class="ghost" @click="saveAll()">Save all</button>
                <button @click="startRun"
                  :disabled="run && (run.status==='running' || run.status==='queued')">Launch</button>
                <button class="danger" @click="stopRun"
                  :disabled="!run || (run.status!=='running' && run.status!=='queued')">
                  {{ run && run.status==='queued' ? 'Cancel' : 'Stop' }}</button>
              </div>
            </div>

            <div v-if="run" style="margin-top:14px">
              <div class="row" style="justify-content:space-between;margin-bottom:6px">
                <span class="status-badge" :class="'s-'+run.status">{{ run.status }}</span>
                <span class="muted" v-if="run.status==='queued'">
                  position {{ run.position }} of {{ run.queued_count }}<span v-if="run.eta_seconds!=null">
                    · starts in ~{{ fmtDur(run.eta_seconds) }}</span></span>
                <span class="muted" v-else-if="run.total">{{ run.progress }} / {{ run.total }}<span
                  v-if="run.eta_seconds!=null"> · ~{{ fmtDur(run.eta_seconds) }} left</span></span>
                <span class="muted" v-else-if="run.status==='running'">initializing…</span>
              </div>
              <div class="progress" :class="{indet: progressIndet || run.status==='queued'}">
                <div class="bar" :style="{width: (run.percent||0)+'%'}"></div>
                <div class="label">{{ run.percent!=null ? run.percent+'%' : '' }}</div>
              </div>
              <div v-if="run.status==='done' && run.output_dir" style="margin-top:12px" class="row">
                <span class="ok">✓ Results: <span class="mono">{{ run.output_dir }}</span></span>
                <a :href="downloadUrl(run.output_dir)"><button>Download results (zip)</button></a>
              </div>
              <h3 style="margin-top:16px">Log</h3>
              <div class="logbox mono">{{ (run.log||[]).join('\\n') }}</div>
            </div>
            <p v-else class="muted" style="margin-top:10px">
              Saves all configs, then runs the simulator and streams progress here.</p>
          </div>

          <div class="card" v-if="run && run.run_folder && (run.status==='running' || run.status==='done')">
            <h2>Live plot</h2>
            <p class="muted">Measurement markers for completed parameter points, updating as
              the run progresses. When it finishes, full axis/slice controls appear.</p>
            <plot-panel :key="run.run_folder + run.status" :run-name="run.run_folder"
              :api="apiFn" :notify="notifyFn"
              :live="run.status==='running'" :refresh-signal="run.progress"></plot-panel>
          </div>

          <div class="card">
            <div class="row" style="justify-content:space-between">
              <h2 style="margin:0">Past results</h2>
              <button class="ghost" @click="loadResults">Refresh</button>
            </div>
            <table class="sweep" v-if="results.length">
              <thead><tr><th>Folder</th><th>HDF5</th><th>Size</th><th></th></tr></thead>
              <tbody>
                <tr v-for="r in results" :key="r.name">
                  <td class="mono">{{ r.name }}</td>
                  <td class="muted">{{ r.h5_files.join(', ') || '—' }}</td>
                  <td class="muted">{{ fmtSize(r.size) }}</td>
                  <td><a :href="downloadUrl(r.name)"><button class="ghost">Download</button></a></td>
                </tr>
              </tbody>
            </table>
            <p v-else class="muted">No results yet.</p>
          </div>
        </div>
      </template>
    </div>

    <!-- matrix/vector peek popup -->
    <div v-if="peek" class="modal-overlay" @click.self="closePeek">
      <div class="modal">
        <div class="modal-head">
          <span class="mono">{{ peek.name }}
            <span class="muted">({{ peek.rows }}×{{ peek.cols }}, {{ fmtSize(peek.size) }})</span>
          </span>
          <div class="row">
            <button class="ghost" @click="peekToEditor">Edit in Files</button>
            <button class="ghost" @click="closePeek">Close</button>
          </div>
        </div>
        <div class="modal-body">
          <table v-if="peek.asTable" class="matrix mono">
            <tr v-for="(r,ri) in peek.grid" :key="ri">
              <td v-for="(c,ci) in r" :key="ci">{{ c }}</td>
            </tr>
          </table>
          <pre v-else class="mono">{{ peek.content }}</pre>
          <p v-if="peek.truncated" class="muted">Preview truncated (large file).</p>
        </div>
      </div>
    </div>

    <!-- noise config popup -->
    <div v-if="cfgModal" class="modal-overlay" @click.self="cfgModal=null">
      <div class="modal">
        <div class="modal-head">
          <span class="mono">{{ cfgModal.title }} — noise config</span>
          <button class="ghost" @click="cfgModal=null">Close</button>
        </div>
        <div class="modal-body"><pre class="mono">{{ cfgModal.json }}</pre></div>
      </div>
    </div>

    <div v-if="toast" class="toast" :class="{err: toastErr}">{{ toast }}</div>
  </div>`
}).mount('#app');
