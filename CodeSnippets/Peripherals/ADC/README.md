# Snippets: ADC Peripheral Snippets

This folder contains two snippets that demonstrate two different ways to interact with the ADC peripheral when coding for Azure Sphere. Additionally, this folder also includes a Linux snippet which is provided to help customers who come from a Linux background, to understand the semantic differences in interacting with the ADC peripheral when coding for Azure Sphere versus coding for a generic Linux system.

All the snippets provide examples of the same ADC functionality, that is, how to open an ADC, get the sample bit count and read a value.

The [ADC sample application](https://github.com/Azure/azure-sphere-samples/tree/main/Samples/ADC/) demonstrates analog-to-digital conversion on the MT3620 high-level core. For more information, see [Use ADCs in High-level applications](https://learn.microsoft.com/azure-sphere/app-development/adc#adc-access) documentation.


## Snippets

### Simplified Functions
This snippet demonstrates how to interact with the ADC using the simplified functions provided by Azure Sphere.
 * [SimplifiedFunctions Snippet](SimplifiedFunctions)

### Advanced Functions
This snippet demonstrates how to interact with the ADC using the advanced functions provided by Azure Sphere. In this snippet, Linux ioctls are used to communicate with the peripheral directly.
This can be used by an intermediate Linux developer who is used to working with peripherals using the Linux libraries.
 * [AdvancedFunctions Snippet](AdvancedFunctions)

### Linux sysfs nodes
This snippet is entirely Linux-based and will not run on Azure Sphere. We provide it as a reference to help you understand the differences in semantics between interacting with the peripheral when coding for Azure Sphere versus coding for a generic Linux system. It may be particularly helpful if you have a Linux background but are new to Azure Sphere.

 * [LinuxSysfsNodes Snippet](LinuxSysfsNodes)
