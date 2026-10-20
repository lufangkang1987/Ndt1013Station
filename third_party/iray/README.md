# Project-local iRay SDK

Source: 903-340-32_NDT_SDK_ReleasePackage_4.4.4.10192_1117.
Imported: 2026-10-20. Vendor files are copied without modification.

| Directory | Source in release package | Purpose |
|---|---|---|
| include | Dev/cpp | Public SDK headers |
| bin/x64 | Dev/bin/x64 | Full x64 runtime directory, license and supporting files |
| work_dir/NDT1013LA | Dev/work_dir/NDT1013LA | Product configuration template |
| redist | Tools/env_setup/microsoft/vcredist_x64_vs2013.exe | VC++ 2013 x64 installer; install manually when required |

manifest.json records relative paths, sizes and SHA-256 hashes of the imported vendor files. It excludes this README and the manifest itself. Original copyrights and license conditions continue to apply; inclusion here does not grant additional redistribution rights.

Both build systems use this directory directly; the original release-package folder is not required to build the project. Qt and MSVC remain external build prerequisites.

Runtime libraries are copied to the application's sdk subdirectory. The work directory is copied to work_dir/NDT1013LA. The license file is preserved as supplied; device/PC authorization must still be verified on the deployment machine. Configuration templates are not device-specific calibration data. Keep field-specific working configurations and calibration templates separate from these vendor originals.
