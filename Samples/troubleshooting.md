# Troubleshooting the sample Azure Sphere apps

This page provides information that might help if you encounter problems building and running the sample applications.

## Problems building in Visual Studio using CMake

- If a problem occurs during CMake cache generation, a CMake error appears in the Error List window along with a message explaining what went wrong. You might have to click on the outlined arrow to the left of the error icon to see the full CMake error message. Common errors are:

   `CMake Error at C:/Program Files (x86)/Microsoft Azure Sphere SDK/CMakeFiles/AzureSphereToolchainBase.cmake:38 (MESSAGE): API set "2" is not valid.  Valid API sets are: ["3", "4", "4+Beta2001"]`

   This means that the application targets an API set that no longer exists, and will have to be updated to target one of the API sets listed in the array.

   `CMake Error at C:/Program Files (x86)/Microsoft Azure Sphere SDK/CMakeFiles/AzureSphereToolchainBase.cmake:61 (MESSAGE): E:/AppSamples/LocalSamples/GPIO/GPIO_HighLevelApp/../../../HardwareDefinitions/mt3620_rdb/sample_appliance.json does not exist`

   This means that CMake was unable to find the hardware directory, likely because it wasn't cloned with the sample, or because the path is wrong.

To fix these and other cache generation errors, modify the CMakeSettings.json file. In Visual Studio, open the file and click on the "Edit JSON" link in the top right corner of the window that appears. Edit the variables inside the environment block at the top of the file to fix the error specified in the message, then save to trigger the CMake cache to regenerate.

Errors during compilation will show up in the Error List as expected.
