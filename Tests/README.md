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

## Adding Custom Test Directories

To add your own test directory with ETL files and gold CSV files:

### Method 1: Using local runsettings file (Recommended)

1. Copy `PresentMonTests.runsettings.template` to `PresentMonTests.local.runsettings`
2. Edit the `PRESENTMON_ADDITIONAL_TEST_DIR` path to point to your test directory
3. In Visual Studio, go to **Test** → **Configure Run Settings** → **Select Solution Wide runsettings File**
4. Select your `PresentMonTests.local.runsettings` file

### Method 2: Command Line Parameter

Run the test executable directly with the `--opttestdir` parameter:
```
PresentMonTests.exe --opttestdir="C:\path\to\your\tests"
```

### Method 3: Environment Variable

Set the `PRESENTMON_ADDITIONAL_TEST_DIR` environment variable:
```cmd
set PRESENTMON_ADDITIONAL_TEST_DIR=C:\path\to\your\tests
PresentMonTests.exe
```

### Test Directory Structure

Your test directory should contain:
- ETL files (e.g., `test_case.etl`)
- Corresponding CSV files (e.g., `test_case.csv`, `test_case_v1.csv`, etc.)

The test system will automatically discover and register tests for each ETL/CSV pair.

### Notes

- The `.local.runsettings` files are ignored by git, so your local paths won't be committed
- Tests are dynamically registered at runtime based on the files found
- Each CSV file creates a separate test case in Test Explorer
