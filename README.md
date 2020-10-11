# MewtwoMegaEvo
![avatar](https://repository-images.githubusercontent.com/295927869/ddd5ba00-fb6d-11ea-9d3c-e3cd43139e62)

Mewtwo is a density matrix solver based on the Von-Neumann equation. Provided the simulator with time dependend hamiltonian 
and the initial states of the system, the simulator can output the time evolution of the density matrix.


Mewtwo refactoring project;
Re-developed from DM_Solver

## Installation

### Compiler & CMake Requirement
1. gcc-8.1+
2. CMake 11.0+

### Thrid party libs
1. Armadillo 9.x+
2. openMP (Embeded in gcc)
3. Json (nlohmann::json Fetched by CMake)
4. HDF5

## Design
![archetechture](docs/archetechture.png)
### Parallelization

### Job slicing for high performance computer

### Runtime reload of sweeping parameters

## How to use
### Command args

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
  "observables": ["IZ"],
  "init_states": ["IZ"],
  "iterations" : 1,
  "step_size": 1e-8,
  "sequence": "F(T/4)-X1pi-F(T/2)-X1pi-F(T/4)-M",
  "sweep_param_name":"Gate:F:pulse_width",
  "sweep_val_path":"/Users/rockysu/MewtwoMegaEvo/configfiles/param_vec"
}
```


#### gate_config.json


#### hamiltonian.json

#### Symbolic/External matrix loading

#### Symbolic Sequence definations
##### Sequence string grammer
1. Sub sequence wrapped by square braket "[]"
2. "[]^n" will repeat the sub sequence for n times.
3. Each symbolic gate in the sequence is separated by '-'
- E.x.
```
Sequence_string: F(T/4)-[U1-U2]^2-U2-F(T/4)
Result: F(T/4), U1, U2, U1, U2, U2, F(T/4)
Where, gate symbols are decomposed to tag-param pair, for e.x., F(T/4) will be decomposed to "F" : "T/4
```

## Output data