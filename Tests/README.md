# PresentMon Tests

PresentMon testing is primarily done by having a specific PresentMon build analyze a collection of ETW logs and ensuring its output matches the expected result.  The PresentMonTests application will add a test for every .etl/.csv pair it finds under a specified root directory.

`Tools\run_tests.cmd` will build all configurations of PresentMon, and use PresentMonTests to validate the x86 and x64 builds using the contents of the Tests\Gold directory.


#### PresentMonTestEtls Coverage

The ETW logs provided in the PresentMon repository were chosen to minimally cover as many different present paths as possible.  These logs currently exercise the following PresentMon paths:

| PresentMode | TRACK_PRESENT_PATH | Test case |
| ----------- | ----------- | -- |
| Composed_Composition_Atlas         | 000000000000000001110000000001000 | 1 |
|                                    | 000000000000000001110001000001000 | 1, 3, 4 |
| Composed_Copy_CPU_GDI              | 000000000000000011100001000001010 | 3, |
| Composed_Copy_GPU_GDI              | 000000000000000011110001000001000 | 1, |
|                                    | 000100000000000011100001000001000 | 3 |
|                                    | 000100000000000011110001000001000 | 1, 4 |
|                                    | 100000000000000011110001000001000 | 0, 1, 3 |
|                                    | 100100000000000011110001000001000 | 0, 1, 2, 3, 4 |
| Composed_Flip                      | 000011111000000001110001000001001 | 1, 2, 3, 4 |
|                                    | 010011111000000001110001000001001 | 0, 1, 2, 3, 4 |
| Hardware_Composed_Independent_Flip | 000010011000000001110001000111001 | 1 |
|                                    | 000010011000000001110001001111001 | 1 |
|                                    | 000010011000000001110101000111001 | 3 |
|                                    | 000010011000000001111001000111001 | 0, 1, 3 |
| Hardware_Independent_Flip          | 000010011000000001110011000001001 | 4 |
|                                    | 000010011000000001111011000001001 | 4 |
| Hardware_Legacy_Flip               | 000000000000000000000000101111001 | 0 |
|                                    | 000000000000000000000001100001001 | 1 |
|                                    | 000000000000000000000001100111000 | 2 |
|                                    | 000000000000000000000001100111001 | 0, 3 |
|                                    | 000000000000000000000001101111001 | 0 |
|                                    | 000000000000000000001001100111000 | 2 |
|                                    | 000000000000000000001001100111001 | 0, 1, 2, 3 |
|                                    | 000000000000000000011001010111000 | 1 |
|                                    | 000000000000000000011011010001000 | 4 |
|                                    | 000000000000000000011011010001001 | 4 |
| Unknown                            | 000000000000000000000000000000001 | 0, 1, 2, 3, 4 |
|                                    | 000000000000000000010000000000001 | 4, |
|                                    | 100000000000000000000000000000000 | 2, |

The following expected cases are currently missing (WIP):

- PresentMode==Hardware_Legacy_Copy_To_Front_Buffer
- [2,17-23] All Windows7 paths
- [30] Non-Win7 Microsoft_Windows_Dwm_Core::FlipChain_(Pending|Complete|Dirty) with previous Microsoft_Windows_Dwm_Core::PresentHistory[Detailed]::Start
