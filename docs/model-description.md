---
layout: default
title: Model documentation
---

# Model documentation (ODD)
The model description follows the ODD (Overview, Design concepts, Details) protocol for describing individual- and agent-based models (Grimm et al. 2006, 2010). 

## 1. Purpose
GRASSMIND is a **process-oriented, individual-based grassland model** being developed to explore the relation of grassland biodiversity with ecosystem functioning and the response to its environment (e.g. weather and soil) and management. Instead of describing a grassland community by a single uniform entity (e.g. average plant), the model tracks many individual plants (organised into cohorts of similar individuals) and lets them interact with each other and their shared environment. The resulting behaviour at the population-, community- and ecosystem-level (i.e. biomass production, species composition, carbon and nitrogen fluxes) is not prescribed but _emerges_ from the behaviour and interactions of multiple individual plants.

The model's purpose is thereby twofold:
* **Mechanistic explanation:** To understand how grassland observations at the population-, community- and ecosystem-level can emerge from individual-level physiological and demographic processes and interactions.
* **Scenario analysis:** To evaluate effects of environmental variability and change (e.g. climate extremes) and management regimes on plant diversity, yield and various ecosystem functions.

> **Why does an individual-based approach make sense for grasslands?**
> In grasslands, individual plant size matters enormously. Tall plants can shade shorter neighbours. Deep-rooted plants can access soil moisture that shallow-rooted plants will never reach. Fast-growing plants (with early-establishing seedlings) can pre-empt space before others germinate. Such size-dependent competitive advantages cannot be captured in models that represent the entire grassland community by an average plant. The GRASSMIND model explicitly represents intra- and inter-specific plant size differences within the community, with competition being dependent on species-specific plant traits and the current state of each plant cohort (e.g. number of plants of similar size).

GRASSMIND uses the **gap-model approach** that has a long track record in forest ecology (JABOWA - Botkin et al. 1972, FORMIND - Köhler & Huth 1998). In gap models, a small patch of vegetation is treated as the basic interaction unit. All plants on a patch interact for resources (like light, water, and nitrogen) without having an explicitly assigned location within the patch. 

With respect to its processes, the GRASSMIND model combines biodiversity and biogeochemistry. Three elemental cycles are explicitly considered:
* **Carbon cycle:** from atmospheric CO<sub>2</sub> fixed by photosynthesis, through plant growth and tissue turnover, to litter decomposition and soil organic matter formation. The ecosystem carbon balance emerges from the difference between gross photosynthesis and all respiration plus decomposition losses.
* **Nitrogen cycle:** plants take up mineral nitrogen from soil and incorporate it into biomass at species-specific C:N ratios. When plant tissue senesces and is decomposed by soil microbes, nitrogen is either re-mineralised (made available again) or temporarily immobilised in microbial biomass. The model tracks nitrogen from soil mineral pools, through plant biomass, to litter and back.
* **Water cycle:** precipitation, snow, canopy interception, surface runoff, percolation through soil layers, plant transpiration, and bare-soil evaporation are all simulated. Plants transpire in proportion to their productivity, and water shortage directly reduces photosynthesis.

**Biodiversity** is built into the model through the concept of _plant functional types (PFTs)_. A PFT is a group of species that behave similarly with respect to growth, resource use, and reproduction. Each PFT is characterised by a set of trait parameters (see section 8). Plants of the same PFT share identical trait values but may differ in age, size, and current resource status because they have individual histories. In this way, GRASSMIND combines the tractability of functional types with the diversity of individual histories.

## 2. Entities, state variables, and scales
### 2.1 Entities

#### 2.1.1 The plant cohort and its individuals
Plant cohorts are the primary ecological actors in the model. Each plant cohort consists of a number of equivalent individual plants having identical states (i.e. _state variables_) and traits (i.e. _model parameters_). Because of identical individuals within each cohort, state variables and traits correspond to just one representative individual plant for each cohort. Table 1 shows all _state variables_ and Table 2 the _traits_ of a representative individual plant of a cohort. While some _state variables_ are directly calculated by model processes (see section 3), others are derived from these _process-based state variables_ using _allometric relationships_ and _traits_.  

| State variables   | Unit          | Determined by       | How?                                |
|:------------------|:--------------|:--------------------|:------------------------------------|
| Age               | Days          | Model time step     | Daily increment by one              |
| Shoot biomass     | g(ODM) per m² | Growth process      | NPP allocation (section xx)         |
| Root biomass      | g(ODM) per m² | Growth process      | NPP allocation (section xx)         |
| Plant biomass     | g(ODM) per m² | Shoot and root      | Sum of shoot and root biomass       |
| Plant height      | cm            | Allometric relation | Based on shoot biomass (section xx) |

> All plant cohorts are organized as one collective in a shared pointer list in GRASSMIND (called ```js COMMUNITY::allPlants ```).

#### 2.1.2 The environment

The environment is characterized by daily changing (1) weather conditions and (2) soil carbon-nitrogen-water dynamics. Environmental conditions are horizontally homogeneous within the simulation area of one patch but differ vertically. Therefore, the aboveground patch space is organized in 1-cm height layers (up to 300 cm) while the belowground space is organized in 10-cm soil layers (up to 200 cm depth; in total 20 soil layers).
...
_table state variables_

#### 2.1.3 The management

The management regime is prescribed before the simulation start (see section xx on model parameters) and encompass day-specific actions like 
* mowing: date(s) of the cutting event(s) and cutting height (in cm)
* fertilization: date(s) of the fertilization and amount of nitrogen (in g(mineral N) per m²)
* irrigation: date(s) of irrigation events and amount of water applied (in mm)
* (re-)seeding: date(s) of initial or re-seeding events and the amount of seeds applied per PFT

### 2.2 Spatial and temporal scales
Each GRASSMIND simulation runs on a fixed area of **1 m²**. This is the patch on which all plants interact with each other. The patch is homogeneous which means there are no explicit spatial positions of individuals within it. 

The model runs at a **daily time step (Δt = 1 day)**. Every day the biological mechanisms and soil processes are executed in a fixed sequence driven by day-specific environmental and management data (see section xxx). Simulations typically run for multiple years to decades to capture vegetation succession and soil organic matter accumulation.

| Scale element         | Model definition | Implication                                         |
|:----------------------|:-----------------|:----------------------------------------------------|
| Temporal resolution   | 1 day (= 1 step) | All vegetation and soil processes are daily updates |
| Temporal extent       | 1 Jan _firstYear_ to 31 Dec _lastYear_ | Length of simulation period defined in configuration file (see section xxx) and converted to day count (starting at 1 for the 1 Jan _firstYear_).|
| Horizontal space (resolution and extent) |1 m² = 10000 cm²  | No explicit positions of individual plants on the simulation area. Resources used for plant growth are homogeneously distributed. |
| Vertical canopy space | 1 cm layers up to _MAXIMUM_HEIGHT_LAYER = 300_ | Light attenuation and shading of plants are resolved vertically.| 
| Vertical soil space   | 20 soil layers of 10 cm each | Water and nitrogen uptake and distribution are soil-layer-specific.|
 
## 3. Process overview and scheduling

Each simulation day starts first by resetting day-level accumulator variables and reading the day's weather inputs. Then a fixed sequence of biological and physical model processes is executed as follows: 
1. recruitment and emergence of plant seedlings
2. plant senescence and mortality
3. gross production (GPP, i.e. photosynthesis incl. shading)
4. soil water uptake and GPP limitation by water
5. plant respiration (for maintenance and growth)
6. net production (NPP)
7. soil nitrogen uptake and NPP limitation by nitrogen
8. allocation for plant growth
9. plant size update
10. management actions
11. update water balance
12. update carbon and nitrogen decomposition cycle

Within each process of growth, mortality and management (2.-10.), the model loops over all cohorts in the community. 
..._pseudo_code_...

## 4. Design concepts

_Basic principles_

_Emergence_

_Adaptation_

_Objectives_

_Learning_

_Prediction_

_Sensing_

_Interaction_

_Stochasticity_

_Collectives_

_Observation_

_Explanation_

## Citation

Add the recommended citation and links to related publications here.
