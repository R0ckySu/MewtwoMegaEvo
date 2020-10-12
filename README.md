# MewtwoMegaEvo
![avatar](https://repository-images.githubusercontent.com/295927869/ddd5ba00-fb6d-11ea-9d3c-e3cd43139e62)

Mewtwo is a density matrix solver based on the Von-Neumann equation. Provided the simulator with time dependend hamiltonian 
and the initial states of the system, the simulator can output the time evolution of the density matrix.

## Installation

### Compiler & CMake Requirement
1. gcc-8.1+
2. CMake 11.0+

### Thrid party libs
1. [Armadillo](http://arma.sourceforge.net/download.html) 9.x+ 
2. openMP (Embeded in gcc, No need to install manually)
3. Json (nlohmann::json Fetched by CMake, No need to install manually)
4. [HDF5](https://www.hdfgroup.org/solutions/hdf5/)
5. [RTTR](https://www.rttr.org)

### For Mac
If you have homebrew installed already, pre-compiled libs including armadillo, hdf5 can be installed easily by brew install command.

### For Linux (HPC user without root privilege)

### For Windows


## Design
![archetechture](docs/archetechture.png)

### Parallelization

### Job slicing for high performance computer
#### Job slicing strategy
| strategy  |  description |
| ----------- | ----------- | 
| linspace |  |
| logspace |  |
| inv_logspace |  |

### Runtime reload of sweeping parameters

## User guide
### Command args
| arg  | arg name  | description |
| ----------- | ----------- | ----------- |
| -g | num_job_group | Num of the jobs [Job slicing for HPC](#Job-slicing-for-high-performance-computer)|
| -i | job_id | Index of the current job [Job slicing for HPC](#Job-slicing-for-high-performance-computer)  |
| -o | output_folder | Specifies the simulation result export path, /sim_results folder will be created at current dir by default. |
| -c | config_folder| Specifies the simulation configuration folder, /config_files at current dir by default|
| -t | timestamp | Force specifying the time stamp of the task, generated automatically by default |

### .json Configuration
All of the simulation configurations are controlled and defined by the .json file.
#### sim_config.json
sim_config.json provides all the general configurations of the simulation.
```json5
{
  "task_name": "TwoQubitQNS",
  "log_level": 4,
  "record_unitary": false,
  "record_all_meas": true,
  "system_dim": 4,
  "observables": ["IZ","IX"],
  "init_states": ["IZ","IY"],
  "iterations" : 64,
  "step_size": 1e-8,
  "sequence": "F(T/4)-X1pi-F(T/2)-X1pi-F(T/4)-M",
  "sweep_param_name":"Gate:F:pulse_width",
  "sweep_val_path":"/Users/rockysu/MewtwoMegaEvo/configfiles/param_vec"
}
```
| field name  |  description |
| ----------- | ----------- |
| task_name | Defines task name |
| log_level | Controls the level of detail of the log output. Larger num will output more detailed log |
| record_unitary | Set to true will output all of the unitary matrix generated on simulation (Time consuming) |
| record_all_meas | Set to true, measurement will be taken at every time point (Time consuming) |
| system_dim | Defines the dimension of the Hilbert space |
| observables | List of the observables (See also: Symbolic/External matrix loading) |
| init_states | List of the density matrices at t=0 (See also: Symbolic/External matrix loading) |
| iterations | Defines the num of iterations. (Same num of the noise realisations will be load to simulation, see also: Noise Hamiltonian) |
| step_size | Time resolution of the simulation in second. |
| sequence | Symbolic sequence string. (See also: [Symbolic Sequence definitions](#symbolic-sequence-definations)) |
| sweep_param_name | All the numeric fields in the config files could be charged with parametric sweeping. See parametric sweeping |
| sweep_val_path | Specifies the source data for the sweeping parameter. See also: parametric sweeping |

#### gate_config.json
gate_config.json provides all the gate prototypes for building symbolic sequences for our simulation task.
Gate prototypes will only define the timing information and its corresponding Hamiltonians.
```json
{  
  "gate_defs": [
    {
      "tag": "X1pi",
      "hamiltonians": ["X1"],
      "pulse_width": 1e-4
    },
    {
      "tag": "X2pi",
      "hamiltonians": ["X2"],
      "pulse_width": 1e-4
    },
    {
      "tag": "F",
      "hamiltonians": [],
      "pulse_width": 1e-4
    }
  ]
}
```

| field name  |  description |
| ----------- | ----------- | 
| tag | Defines the symbol of the gate (See also: Symbolic Sequence definations) |
| hamiltonians | Mapping the gate to its corresponding Hamiltonians by its tag. |
| pulse_width | Defines the gate operation time length |

#### hamiltonian.json
```json
{
  "hamiltonian_prototype_defs": [
    {
      "tag": "X1",
      "type": "mw",
      "enable": true,
      "amplitude":6.2831852e5,
      "freq": 0,
      "phase": 0,
      "h_pauli_mat": "IX",
      "waveform_path":""
    },
    {
      "tag": "X2",
      "type": "mw",
      "enable": true,
      "amplitude":1e4,
      "freq": 0,
      "phase": 1.5707963,
      "h_pauli_mat": "XI",
      "waveform_path":""
    },
    {
      "tag": "noise1",
      "type": "noise",
      "enable": false,
      "amplitude":1e2,
      "h_pauli_mat": "IZ",
      "lag_time": 1e-6,
      "waveform_path":"/Users/rockysu/mewtwo/mewtwo_executable/Darwin/NoiseData/PinkNoiseQ11#.csv"
    },
    {
      "tag": "noise2",
      "type": "noise",
      "enable": false,
      "amplitude":1e2,
      "h_pauli_mat": "ZI",
      "lag_time": 1e-6,
      "waveform_path":"/Users/rockysu/mewtwo/mewtwo_executable/Darwin/NoiseData/PinkNoiseQ21#.csv"
    }
  ]
}
```

| field name  |  description |
| ----------- | ----------- | 
| tag | Tag of the hamiltonian prototype |
| type | Hamiltonian type, could be specified as static, mw, awg and noise. (See also: Hamiltonian types) |
| enable | Switch for hamiltonian turning on/off |
| amplitude | Scaling the amplitude of the waveform |
| h_pauli_mat | Defines the hamiltonian matrix (See also: Symbolic/External matrix loading) |
| waveform_path | Specify the external waveform data file path(in csv format) |

#### Parametric Sweeping


#### Symbolic/External matrix loading
Complex matrices in the json config files could be either defined as SU(n) spinor symbols or loaded from external csv file.
- e.x.
1. Symbol "XY" will be parsed to be the tensor product of pauli matrix X and Y. Spinor symbol pattern will be identified automatically. 
2. Symbol "rho1" is not identified as spinor symbol, so it will be loaded from ${config_folder}/rho1 in csv format.

#### Symbolic Sequence definations
##### Gate Symbol

##### Measurement Marker

##### Sequence string grammer
1. Sub sequence wrapped by square braket "[]"
2. "[]^n" will repeat the sub sequence for n times.
3. Each symbolic gate in the sequence is separated by '-'
- e.x.
```
Sequence_string: F(T/4)-[U1-U2]^2-U2-F(T/4)
Result: F(T/4), U1, U2, U1, U2, U2, F(T/4)
Where, the gate symbols are decomposed to tag-param pair, for e.x., F(T/4) will be decomposed to "F" : "T/4
```
NOTE: Symbol "M" is reserved as measurement marker.

#### Hamiltonian types
##### Static Hamiltonian
##### Microwave Hamiltonian
##### AWG Hamiltonian
Coming Soon
##### Noise Hamiltonian

## Output data
Output result data will be saved as HDF5 file.

Data structure tree diagram:
```
|-Dataset|
         |-param1-|
         |-param2-|
            ...
         |-paramN-|
                  |-time_vec-|
                  |-Observable1-|
                  |-Observable2-|
                       ...
                  |-ObservableN-|
                                |-rho1-|
                                |-rho2-|
                                |-rho3-|
                                  ...
```