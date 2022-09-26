# Troubleshooting the sample Azure Sphere apps

This page provides information that might help if you encounter problems building and running the sample applications.

## Problems building in Visual Studio using CMake

If a problem occurs during CMake cache generation, CMake errors appear in the **Error List** window along with a message explaining what went wrong. You might have to select the outlined arrow to the left of the error icon to see the full CMake error message. The following are common errors:

* Need to define a valid API set:

    `CMake Error at C:/Program Files (x86)/Microsoft Azure Sphere SDK/CMakeFiles/AzureSphereToolchainBase.cmake:38 (MESSAGE): API set "2" is not valid.  Valid API sets are: ["3", "4", "4+Beta2001"]`

    This means that the application targets an API set that no longer exists, and needs to be updated to target one of the valid API sets listed in the array. To fix this error, modify `TARGET_API_SET` in the CMakeLists.txt file.

* Need to define hardware directory:

    `CMake Error at C:/Program Files (x86)/Microsoft Azure Sphere SDK/CMakeFiles/AzureSphereToolchainBase.cmake:61 (MESSAGE): E:/AppSamples/LocalSamples/GPIO/GPIO_HighLevelApp/../../../HardwareDefinitions/mt3620_rdb/sample_appliance.json does not exist`

    This means that CMake was unable to find the hardware definitions directory, either because it wasn't cloned or copied with the sample, or because the path is wrong. To fix this error, make sure that `TARGET_DIRECTORY` and `TARGET_DEFINITION` are correctly defined in the **azsphere_target_hardware_definition** function in the CMakeLists.txt file.

To fix other cache generation errors, you might also need to modify the CMakePresets.json file. In Visual Studio, open the file and edit the variables inside the `cacheVariables` block to fix the error specified in the message, then save to trigger the CMake cache to regenerate. Errors during compilation will appear in the **Error List** as expected.
