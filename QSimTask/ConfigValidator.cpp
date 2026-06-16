//
// Created by Rocky Su & Claude Fable 5 on 16/06/2026.
//

#include "ConfigValidator.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <regex>
#include <set>
#include <algorithm>
#include <cmath>

// ────────────────────────────────────────────────────────────────────
//  Constructor / Destructor
// ────────────────────────────────────────────────────────────────────

ConfigValidator::ConfigValidator(std::string config_folder)
    : config_folder_(std::move(config_folder)) {}

ConfigValidator::~ConfigValidator() = default;

// ────────────────────────────────────────────────────────────────────
//  Public API
// ────────────────────────────────────────────────────────────────────

bool ConfigValidator::validate_sim_config(const nlohmann::json& json) {
    bool ok = true;

    // ── 1. Required top-level keys ──────────────────────────────────
    ok &= check_required_keys(json, {
        "task_name",
        "log_level",
        "job_slicing_strategy",
        "record_propagator",
        "record_all_meas",
        "record_density_mat",
        "reset_rotating_frame",
        "enable_param_parallel_mode",
        "system_dim",
        "observables",
        "init_states",
        "repeat",
        "step_size",
        "sequence",
        "sweep_param_info"
    }, "sim_config.json");

    if (!ok) return false;  // can't safely check further without keys

    // ── 2. Type checks for top-level scalars ───────────────────────
    auto check_type = [&](const std::string& key,
                          const std::string& type_name,
                          bool (*check_fn)(const nlohmann::json&)) {
        if (json.contains(key) && !check_fn(json[key])) {
            errors_.push_back("sim_config.json: '" + key +
                              "' must be of type " + type_name);
        }
    };

    check_type("task_name",             "string",
               [](const nlohmann::json& j) { return j.is_string(); });
    check_type("log_level",             "integer",
               [](const nlohmann::json& j) { return j.is_number_integer(); });
    check_type("job_slicing_strategy",  "string",
               [](const nlohmann::json& j) { return j.is_string(); });
    check_type("record_propagator",     "boolean",
               [](const nlohmann::json& j) { return j.is_boolean(); });
    check_type("record_all_meas",       "boolean",
               [](const nlohmann::json& j) { return j.is_boolean(); });
    check_type("record_density_mat",    "boolean",
               [](const nlohmann::json& j) { return j.is_boolean(); });
    check_type("reset_rotating_frame",  "boolean",
               [](const nlohmann::json& j) { return j.is_boolean(); });
    check_type("enable_param_parallel_mode", "boolean",
               [](const nlohmann::json& j) { return j.is_boolean(); });
    check_type("system_dim",            "integer",
               [](const nlohmann::json& j) { return j.is_number_integer(); });
    check_type("repeat",                "integer",
               [](const nlohmann::json& j) { return j.is_number_integer(); });
    check_type("step_size",             "number",
               [](const nlohmann::json& j) { return j.is_number(); });
    check_type("sequence",              "string",
               [](const nlohmann::json& j) { return j.is_string(); });

    // ── 3. Array type checks ───────────────────────────────────────
    if (json.contains("observables") && !json["observables"].is_array()) {
        errors_.push_back("sim_config.json: 'observables' must be an array");
    }
    if (json.contains("init_states") && !json["init_states"].is_array()) {
        errors_.push_back("sim_config.json: 'init_states' must be an array");
    }
    if (json.contains("sweep_param_info") && !json["sweep_param_info"].is_array()) {
        errors_.push_back("sim_config.json: 'sweep_param_info' must be an array");
    }

    // ── 4. Validate observables matrix files ───────────────────────
    if (json.contains("observables") && json["observables"].is_array()) {
        for (size_t i = 0; i < json["observables"].size(); ++i) {
            if (!json["observables"][i].is_string()) {
                errors_.push_back("sim_config.json: observables[" +
                                  std::to_string(i) + "] must be a string");
                continue;
            }
            std::string obs = json["observables"][i];
            verify_symbolic_matrix(obs,
                "sim_config.json: observable '" + obs + "'");
        }
    }

    // ── 5. Validate init_states matrix files ───────────────────────
    if (json.contains("init_states") && json["init_states"].is_array()) {
        for (size_t i = 0; i < json["init_states"].size(); ++i) {
            if (!json["init_states"][i].is_string()) {
                errors_.push_back("sim_config.json: init_states[" +
                                  std::to_string(i) + "] must be a string");
                continue;
            }
            std::string st = json["init_states"][i];
            verify_symbolic_matrix(st,
                "sim_config.json: init_state '" + st + "'");
        }
    }

    // ── 6. Validate sweep_param_info entries ───────────────────────
    if (json.contains("sweep_param_info") && json["sweep_param_info"].is_array()) {
        for (size_t i = 0; i < json["sweep_param_info"].size(); ++i) {
            auto& entry = json["sweep_param_info"][i];
            std::string ctx = "sim_config.json: sweep_param_info[" +
                              std::to_string(i) + "]";

            if (!entry.is_object()) {
                errors_.push_back(ctx + " must be an object");
                continue;
            }

            check_required_keys(entry, {"class", "tag", "property"}, ctx);

            if (entry.contains("class") && entry["class"].is_string()) {
                std::string cls = entry["class"];
                if (cls == "Sequence") {
                    if (!entry.contains("string_file")) {
                        errors_.push_back(ctx +
                            ": Sequence-class entry requires 'string_file'");
                    } else if (entry["string_file"].is_string()) {
                        std::string sf = entry["string_file"];
                        verify_symbolic_matrix(sf,
                            ctx + ": string_file '" + sf + "'");
                    }
                } else if (cls == "Gate" || cls == "Hamiltonian") {
                    if (!entry.contains("val_file") && !entry.contains("string_file")) {
                        errors_.push_back(ctx +
                            ": '" + cls + "'-class entry requires "
                            "'val_file' or 'string_file'");
                    }
                    // Check val_file existence
                    if (entry.contains("val_file") && entry["val_file"].is_string()) {
                        std::string vf = entry["val_file"];
                        verify_symbolic_matrix(vf,
                            ctx + ": val_file '" + vf + "'");
                    }
                    if (entry.contains("string_file") && entry["string_file"].is_string()) {
                        std::string sf = entry["string_file"];
                        verify_symbolic_matrix(sf,
                            ctx + ": string_file '" + sf + "'");
                    }
                } else {
                    errors_.push_back(ctx + ": unknown class '" + cls +
                        "' (expected: Gate, Hamiltonian, or Sequence)");
                }
            }
        }
    }

    // ── 7. Validate dimension consistency ───────────────────────────
    if (json.contains("system_dim") && json["system_dim"].is_number_integer()) {
        int dim = json["system_dim"];
        if (dim < 1 || dim > 256) {
            warnings_.push_back("sim_config.json: system_dim = " +
                std::to_string(dim) + " is unusual; typical range is 2-16");
        }
    }

    return ok;
}

// ────────────────────────────────────────────────────────────────────

bool ConfigValidator::validate_gate_config(const nlohmann::json& json) {
    bool ok = true;

    // ── 1. Required top-level key ──────────────────────────────────
    ok &= check_required_key(json, "gate_defs", "gate_config.json");

    if (!ok || !json["gate_defs"].is_array()) {
        if (json.contains("gate_defs") && !json["gate_defs"].is_array())
            errors_.push_back("gate_config.json: 'gate_defs' must be an array");
        return false;
    }

    // Track gate tags to detect duplicates
    std::set<std::string> gate_tags;

    // ── 2. Validate each gate definition ───────────────────────────
    for (size_t i = 0; i < json["gate_defs"].size(); ++i) {
        auto& gate = json["gate_defs"][i];
        std::string ctx = item_context("gate_config.json", "gate_defs", i, gate);

        if (!gate.is_object()) {
            errors_.push_back(ctx + " must be an object");
            continue;
        }

        check_required_keys(gate, {
            "tag", "type", "hamiltonians",
            "pulse_width", "shift_time", "ext_shaped_sig_path"
        }, ctx);

        // Type check
        if (gate.contains("type") && gate["type"].is_string()) {
            std::string gtype = gate["type"];
            if (gtype != "switch" && gtype != "shaped" && gtype != "sticky") {
                warnings_.push_back(ctx + ": unknown gate type '" + gtype +
                    "' (expected: switch, shaped, or sticky)");
            }
        }

        // Detect duplicate gate tags
        if (gate.contains("tag") && gate["tag"].is_string()) {
            std::string tag = gate["tag"];
            if (gate_tags.count(tag)) {
                errors_.push_back(ctx + ": duplicate gate tag '" + tag + "'");
            }
            gate_tags.insert(tag);
        }

        // hamiltonians array
        if (gate.contains("hamiltonians") && gate["hamiltonians"].is_array()) {
            for (size_t j = 0; j < gate["hamiltonians"].size(); ++j) {
                if (!gate["hamiltonians"][j].is_string()) {
                    errors_.push_back(ctx + ": hamiltonians[" +
                        std::to_string(j) + "] must be a string");
                }
            }
        }

        // pulse_width must be positive when no ext_shaped_sig_path
        if (gate.contains("pulse_width") && gate["pulse_width"].is_number()) {
            double pw = gate["pulse_width"];
            if (pw <= 0.0 &&
                (!gate.contains("ext_shaped_sig_path") ||
                 gate["ext_shaped_sig_path"].is_null() ||
                 (gate["ext_shaped_sig_path"].is_string() &&
                  gate["ext_shaped_sig_path"].get<std::string>().empty()))) {
                errors_.push_back(ctx + ": pulse_width must be > 0 "
                    "(got " + std::to_string(pw) + ")");
            }
        }

        // ext_shaped_sig_path file existence
        if (gate.contains("ext_shaped_sig_path") &&
            gate["ext_shaped_sig_path"].is_string()) {
            verify_ext_shaped_sig_path(
                gate["ext_shaped_sig_path"],
                ctx + ": ext_shaped_sig_path");
        }
    }

    // ── 3. Require at least an "M" measurement gate ────────────────
    // (The code auto-adds an 'M' marker, but we warn if no gates defined)
    if (json["gate_defs"].size() == 0) {
        warnings_.push_back("gate_config.json: no gate definitions found "
            "(a measurement marker 'M' is auto-added by the simulator)");
    }

    return ok;
}

// ────────────────────────────────────────────────────────────────────

bool ConfigValidator::validate_hamiltonian_config(const nlohmann::json& json) {
    bool ok = true;

    // ── 1. Required top-level key ──────────────────────────────────
    ok &= check_required_key(json, "hamiltonian_prototype_defs",
                             "hamiltonian_config.json");

    if (!ok || !json["hamiltonian_prototype_defs"].is_array()) {
        if (json.contains("hamiltonian_prototype_defs") &&
            !json["hamiltonian_prototype_defs"].is_array())
            errors_.push_back(
                "hamiltonian_config.json: 'hamiltonian_prototype_defs' "
                "must be an array");
        return false;
    }

    // Track hamiltonian tags for duplicates
    std::set<std::string> h_tags;

    // ── 2. Known Hamiltonian types and their extra fields ──────────
    // Base fields (all types): tag, type, enable, amplitude,
    //                          h_pauli_mat, waveform_path
    //
    // Extra per-type:
    //   static     — (none extra)
    //   static_RF  — RF_freq_mat
    //   mw         — rising_time, falling_time, freq, phase, chirp_rate
    //   mw_RF      — rising_time, falling_time, freq, phase, chirp_rate,
    //                RF_freq_mat, wave_forward_propagate
    //   awg        — rising_time, falling_time
    //   noise      — lag_time, rand_shift, rand_channel,
    //                num_available_channels

    static const std::set<std::string> kValidTypes = {
        "static", "static_RF", "mw", "mw_RF", "awg", "noise"
    };

    static const std::vector<std::string> kBaseFields = {
        "tag", "type", "enable", "amplitude", "h_pauli_mat", "waveform_path"
    };

    static const std::map<std::string, std::vector<std::string>> kExtraFields = {
        {"static",     {}},
        {"static_RF",  {"RF_freq_mat"}},
        {"mw",         {"rising_time", "falling_time", "freq", "phase", "chirp_rate"}},
        {"mw_RF",      {"rising_time", "falling_time", "freq", "phase",
                        "chirp_rate", "RF_freq_mat", "wave_forward_propagate"}},
        {"awg",        {"rising_time", "falling_time"}},
        {"noise",      {"lag_time", "rand_shift", "rand_channel",
                        "num_available_channels"}}
    };

    for (size_t i = 0; i < json["hamiltonian_prototype_defs"].size(); ++i) {
        auto& h = json["hamiltonian_prototype_defs"][i];
        std::string ctx = item_context("hamiltonian_config.json",
                                       "hamiltonian_prototype_defs", i, h);

        if (!h.is_object()) {
            errors_.push_back(ctx + " must be an object");
            continue;
        }

        // ── 2a. Base fields + type determination ──────────────────
        check_required_keys(h, kBaseFields, ctx);

        std::string htype;
        if (h.contains("type") && h["type"].is_string()) {
            htype = h["type"];
            if (!kValidTypes.count(htype)) {
                errors_.push_back(ctx + ": unknown Hamiltonian type '" +
                    htype + "'. Valid types: static, static_RF, mw, "
                    "mw_RF, awg, noise");
                continue;
            }
        } else {
            continue;  // type missing or wrong — already reported
        }

        // ── 2b. Type-specific extra fields ────────────────────────
        auto it = kExtraFields.find(htype);
        if (it != kExtraFields.end()) {
            for (const auto& field : it->second) {
                check_required_key(h, field, ctx + " (type=" + htype + ")");
            }
        }

        // ── 2c. Duplicate tag check ───────────────────────────────
        if (h.contains("tag") && h["tag"].is_string()) {
            std::string tag = h["tag"];
            if (h_tags.count(tag)) {
                errors_.push_back(ctx + ": duplicate Hamiltonian tag '" +
                                  tag + "'");
            }
            h_tags.insert(tag);
        }

        // ── 2d. Validate h_pauli_mat as symbolic matrix ───────────
        if (h.contains("h_pauli_mat") && h["h_pauli_mat"].is_string()) {
            std::string mat = h["h_pauli_mat"];
            verify_symbolic_matrix(mat, ctx + ": h_pauli_mat '" + mat + "'");
        }

        // ── 2e. Validate RF_freq_mat for RF types ─────────────────
        if ((htype == "static_RF" || htype == "mw_RF") &&
            h.contains("RF_freq_mat") && h["RF_freq_mat"].is_string()) {
            std::string rf_mat = h["RF_freq_mat"];
            verify_symbolic_matrix(rf_mat,
                ctx + ": RF_freq_mat '" + rf_mat + "'");
        }

        // ── 2f. Validate waveform_path ────────────────────────────
        if (h.contains("waveform_path") && h["waveform_path"].is_string()) {
            verify_waveform_path(h["waveform_path"],
                ctx + ": waveform_path");
        }

        // ── 2g. Type-specific semantic checks ─────────────────────
        if (h.contains("enable") && h["enable"].is_boolean()) {
            if (!h["enable"]) {
                // Disabled Hamiltonians are fine — skip further checks
                continue;
            }
        }

        // amplitude should be non-zero for enabled hamiltonians
        if (h.contains("amplitude") && h["amplitude"].is_number()) {
            double amp = h["amplitude"];
            if (amp == 0.0) {
                warnings_.push_back(ctx + ": amplitude is 0 — this "
                    "Hamiltonian will have no effect");
            }
        }

        // noise-specific checks
        if (htype == "noise") {
            if (h.contains("waveform_path") && h["waveform_path"].is_string()) {
                std::string wp = h["waveform_path"];
                if (wp.empty()) {
                    warnings_.push_back(ctx +
                        ": noise Hamiltonian has empty waveform_path — "
                        "no noise signal will be loaded");
                }
            }
        }
    }

    // ── 3. Cross-reference with gate_config (soft check) ───────────
    // We can't access gate_config here, but we note that gate
    // hamiltonian references should exist in this set.
    // This cross-check is done outside in QSimTask::preload() if desired.

    return ok;
}

// ────────────────────────────────────────────────────────────────────
//  Result helpers
// ────────────────────────────────────────────────────────────────────

bool ConfigValidator::is_valid() const {
    return errors_.empty();
}

const std::vector<std::string>& ConfigValidator::get_errors() const {
    return errors_;
}

const std::vector<std::string>& ConfigValidator::get_warnings() const {
    return warnings_;
}

void ConfigValidator::print_report() const {
    // Always print to stderr so the report is never buried by stdout noise
    std::ostream& out = std::cerr;

    out << "\n"
        << "══════════════════════════════════════════════════\n"
        << "         MewtwoMegaEvo Config Validation Report\n"
        << "══════════════════════════════════════════════════\n"
        << "  Config folder : " << config_folder_ << "\n"
        << "  Errors        : " << errors_.size() << "\n"
        << "  Warnings      : " << warnings_.size() << "\n"
        << "──────────────────────────────────────────────────\n";

    if (!errors_.empty()) {
        out << "\n  ERRORS:\n";
        for (const auto& e : errors_) {
            out << "    ✗ " << e << "\n";
        }
    }

    if (!warnings_.empty()) {
        out << "\n  WARNINGS:\n";
        for (const auto& w : warnings_) {
            out << "    ⚠ " << w << "\n";
        }
    }

    if (errors_.empty() && warnings_.empty()) {
        out << "\n  ✓ All configurations passed validation.\n";
    }

    out << "\n══════════════════════════════════════════════════\n"
        << (is_valid() ? "  RESULT: PASS" : "  RESULT: FAIL")
        << "\n══════════════════════════════════════════════════\n"
        << std::endl;
}

void ConfigValidator::throw_on_error() const {
    if (errors_.empty()) return;

    std::ostringstream oss;
    oss << "\n"
        << "══════════════════════════════════════════════════\n"
        << "  FATAL: MewtwoMegaEvo configuration is invalid!\n"
        << "══════════════════════════════════════════════════\n"
        << "  The following " << errors_.size() << " error(s) must be fixed:\n\n";
    for (size_t i = 0; i < errors_.size(); ++i) {
        oss << "  " << (i + 1) << ". " << errors_[i] << "\n";
    }
    if (!warnings_.empty()) {
        oss << "\n  There are also " << warnings_.size()
            << " warning(s) — see report above.\n";
    }
    oss << "\n  Config folder: " << config_folder_ << "\n"
        << "  Please fix the errors above and re-run.\n"
        << "══════════════════════════════════════════════════\n";
    throw std::runtime_error(oss.str());
}

// ────────────────────────────────────────────────────────────────────
//  Private helpers
// ────────────────────────────────────────────────────────────────────

bool ConfigValidator::check_required_key(const nlohmann::json& obj,
                                          const std::string& key,
                                          const std::string& context) {
    if (!obj.contains(key)) {
        errors_.push_back(context + ": missing required key '" + key + "'");
        return false;
    }
    return true;
}

bool ConfigValidator::check_required_keys(const nlohmann::json& obj,
                                           const std::vector<std::string>& keys,
                                           const std::string& context) {
    bool all_present = true;
    for (const auto& k : keys) {
        if (!check_required_key(obj, k, context)) {
            all_present = false;
        }
    }
    return all_present;
}

bool ConfigValidator::verify_symbolic_matrix(const std::string& symbol_name,
                                              const std::string& context) {
    if (symbol_name.empty()) {
        errors_.push_back(context + ": empty symbol name");
        return false;
    }

    // Pattern: optional 'i' prefix followed by I, X, Y, Z chars only
    // These are decoded as Pauli spinors in-memory — no file needed.
    static const std::regex kPauliRegex("^i?[IXYZ]+$");
    if (std::regex_match(symbol_name, kPauliRegex)) {
        // Built-in Pauli spinor — always valid
        return true;
    }

    // Otherwise, must exist as a file relative to config_folder_
    std::string full_path = config_folder_ + "/" + symbol_name;
    if (!file_exists(full_path)) {
        errors_.push_back(context + ": matrix file not found at '" +
                          full_path + "'");
        return false;
    }

    // Quick sanity check on file readability
    std::ifstream test(full_path);
    if (!test.good()) {
        errors_.push_back(context + ": matrix file exists but is not "
                          "readable: '" + full_path + "'");
        return false;
    }
    test.close();

    return true;
}

bool ConfigValidator::file_exists(const std::string& path) const {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

bool ConfigValidator::verify_waveform_path(const std::string& path,
                                            const std::string& context) {
    if (path.empty()) {
        return true;  // empty = no external waveform; OK
    }

    // The path may contain a '#' placeholder for the index,
    // e.g. "./NoiseData/.../file#.csv"
    // Strip the '#' and extension to check if the base file pattern exists.
    std::string check_path = path;

    // Handle '#' placeholder: check if any matching file exists
    size_t sharp_pos = check_path.find('#');
    if (sharp_pos != std::string::npos) {
        // Replace "#" with "0" and check existence
        std::string probe = check_path;
        probe.replace(sharp_pos, 1, "0");
        if (file_exists(probe)) {
            return true;
        }
        // Also try checking the directory containing the pattern
        size_t last_slash = check_path.find_last_of('/');
        if (last_slash != std::string::npos) {
            std::string dir = check_path.substr(0, last_slash);
            if (!file_exists(dir)) {
                // Try relative to config_folder_
                std::string abs_dir = config_folder_ + "/" + dir;
                if (file_exists(abs_dir)) {
                    return true;  // directory exists
                }
            } else {
                return true;
            }
        }
        warnings_.push_back(context + ": could not verify file matching "
            "pattern '" + path + "'. The file should exist with a numeric "
            "index replacing '#' (e.g. file0.csv)");
        return true;  // non-fatal: the pattern is a convention
    }

    // Direct file path check
    if (file_exists(check_path)) {
        return true;
    }

    // Also try relative to config_folder_
    std::string relative_path = config_folder_ + "/" + check_path;
    if (file_exists(relative_path)) {
        return true;
    }

    errors_.push_back(context + ": file not found at '" + path +
                      "' or '" + relative_path + "'");
    return false;
}

bool ConfigValidator::verify_ext_shaped_sig_path(const std::string& path,
                                                   const std::string& context) {
    if (path.empty()) {
        return true;  // no external shaped signal
    }

    // Absolute or relative path check
    if (file_exists(path)) {
        return true;
    }

    // Try relative to config_folder_
    std::string relative_path = config_folder_ + "/" + path;
    if (file_exists(relative_path)) {
        return true;
    }

    errors_.push_back(context + ": shaped signal file not found at '" +
                      path + "' or '" + relative_path + "'");
    return false;
}

// ────────────────────────────────────────────────────────────────────

std::string ConfigValidator::item_context(const std::string& file,
                                           const std::string& array_name,
                                           size_t index,
                                           const nlohmann::json& item) {
    std::string ctx = file + ": " + array_name + "[" +
                      std::to_string(index) + "]";
    if (item.is_object() && item.contains("tag") && item["tag"].is_string()) {
        ctx += " (tag=\"" + item["tag"].get<std::string>() + "\")";
    }
    return ctx;
}
