---
# The content between the beginning and ending "---" is YAML front matter that specifies metadata.
# Text that follows a "#" (number sign) is a YAML comment. NOTE: You should remove all YAML comments from the metadata, but be careful to remove only the comments and not the metadata itself or the "---" delimiters.
# There are seven top-level fields: page_type, languages, products, name, urlFragment, extendedZipContent, and description. All are required in Azure Sphere samples README.md files.
# The values of the first three fields (page_type, languages, and products) should be exactly as shown. The values of the other fields are variable -- see the NOTES and GUIDELINES below.

page_type: sample
languages:
- c
products:
- azure
- azure-sphere
name: Azure Sphere - <Name of sample>
urlFragment: <url-fragment-of-sample>
extendedZipContent:
# Include HardwareDefinitions only if it is appropriate for the sample.
- path: HardwareDefinitions
  target: <target-path>/HardwareDefinitions
# Include the following elements exactly as shown.
- path: .clang-format
  target: .clang-format
- path: BUILD_INSTRUCTIONS.md
  target: BUILD_INSTRUCTIONS.md
- path: Samples/SECURITY.md
  target: SECURITY.md
- path: Samples/troubleshooting.md
  target: troubleshooting.md
description: <abstract>

# ~~~~~~~~~~~~               NOTES and GUIDELINES                ~~~~~~~~~~~~~~
# ~~~~~~~~~~~~      for metadata fields with variable values     ~~~~~~~~~~~~~~
#
# The name, urlFragment, and description fields have variable values, indicated by the text in angle brackets.
# The extendedZipContent field includes HardwareDefinitions only if it is appropriate for the sample.
# Follow the guidelines stated below for these four fields.
#
# name:
#  - Replace <Name of sample> with the name that you want displayed on the sample card in the Samples Browser landing page.
#  - Use sentence case unless the display name includes a proper name.
#  - Spell out words but not acronyms.
#  - Use spaces, not camel case or underscores.
#  - Use only plain-text (no Markdown or HTML).
#  - Examples of values for name:
#       HTTPS cURL Easy not HTTPS_cURL_Easy
#       Inter-core communication not IntercoreComms
#       External MCU update not ExternalMcuUpdate
#
# urlFragment:
#  - Replace <url-fragment-of-sample> with the URL fragment of the published sample.
#  - Use only alphanumeric characters and hyphen. Including any other characters will cause sample ingestion to fail for the entire repository.
#  - The full URL will always be https://docs.microsoft.com/{locale}/samples/Azure/azure-sphere-samples/{urlFragment}.
#  - Setting the urlFragment ensures that the page link is immutable, regardless of the sample's README.md changes.
#
# extendedZipContent:
#  - The extendedZipContent field contains a five-element list. Each element specifies a file or folder to be included in the ZIP file.
#  - Each element in extendedZipContent has the following two fields:
#       path, which specifies the relative source path of the file or folder (path is relative to the azure-sphere-samples folder in the repository).
#       target, which specifies the destination folder within the ZIP file.
#  - All elements except the element for HardwareDefinitions must be included.
#  - The HardwareDefinitions element should be included only if it is appropriate for the sample.
#  - If HardwareDefinitions is included, be sure to replace <target-path> with the appropriate destination path for the particular sample. Use / (forward slash) as a directory separator.
#
# description:
#  - Replace <abstract> with a 150-character statement that describes the sample.
#  - Begin with a verb. For example: "Demonstrates the ... " or "Shows how to ... "
#  - Use only plain-text (no Markdown or HTML).
#
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

---

<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ -->
<!-- ~~                      MARKDOWN TEMPLATE                               ~~
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Text in Markdown, which specifies the content that is to be rendered for display to a customer, begins immediately following the metadata. A Markdown template is provided below, following these comments.

Some notes about the Markdown components:

~ The Markdown begins with the title of the sample, formatted as an H1 heading (# in Markdown). The title is followed by a summary of the sample and a table of the libraries used by the sample.
~ There are five main sections, each of which begins with an H2 heading (## in Markdown), that are required. One or more optional main sections can be included to describe how to build a variation of the sample.
~ Replaceable text is delimited by curly brackets. You must supply this text, which varies with each sample.
~ Examples of the Markdown components begin on line 252.

Be sure to read the following information (provided in the HTML comments after the Markdown template):

~ GUIDELINES FOR THE MARKDOWN COMPONENTS (see HTML comments beginning on line 145)
~ GENERAL FORMATTING GUIDELINES AND NOTES (see HTML comments beginning on line 212)

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ -->

# Sample: {Name of sample}

{One or two sentences that briefly describe what the sample does, followed by one or two paragraphs that provide some details}

The sample uses the following Azure Sphere libraries.

|               Library                 |                   Purpose                      |
|---------------------------------------|------------------------------------------------|
| {name of library formatted as a link} | {Statement of library's purpose in the sample} |

## Contents

|        File/folder         |                Description          |
|----------------------------|-------------------------------------|
| `{name of file or folder}` | {Description of the file or folder} |

## Prerequisites

{Bulleted list of items}

## Setup

{Numbered list of steps}

## Build and run the sample

To build and run this sample, follow the instructions in [Build a sample application](../../BUILD_INSTRUCTIONS.md).

### Test the sample (optional)

{Paragraphs or numbered list of steps}

### Observe the output (optional)

{Paragraphs or numbered list of steps}

## Rebuild the sample to {specify what it will do differently} (optional)

{Description of what the revised sample will do differently}

{Paragraphs, list of steps, or subsections with modification instructions}

{Final paragraph, step, or subsection with build-and-run instructions}

## Troubleshooting (optional)

{Details about how to troubleshoot common problems with this sample}

## Next steps

{Paragraph or bulleted list of statements pointing to relevant information}

<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ -->
<!-- ~~             GUIDELINES FOR THE MARKDOWN COMPONENTS                   ~~
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TITLE OF SAMPLE ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The title is an H1 heading that must begin with "Sample: ", as shown in the Markdown template above. Replace {Name of sample} with the name of your sample. The text that replaces {Name of sample} should adhere to the same guidelines specified for the "name" metadata field.

INTRODUCTION / SUMMARY ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

After the title, briefly state what the sample does. Add one or two brief paragraphs after the introductory statement to provide some details.

LIBRARIES TABLE ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Following the introduction, the libraries used by the sample are listed in a table that has two columns, "Library" and "Purpose". Precede the table with the introductory statement that is shown in the Markdown template above.

~ List the libraries alphabetically and do not include the .h extension in the name of the library.
~ Begin the description with a verb and end with a period.
~ The name of the library must link to the corresponding library page in docs.microsoft.com (the URL is typically https://docs.microsoft.com/azure-sphere/reference/applibs-reference/{library}/{library}-overview).

"CONTENTS" SECTION ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

List, in a table, the primary files and folders that are part of the sample. The table has two columns, "File/folder" and "Description", as shown in the Markdown template above. You do not need to list every file/folder that is included in the sample. List the files/folders that the README.md may reference or that are important for the customer to know about.

~ Enclose the file/folder name in single back ticks (`), which formats the text as inline code in Markdown.
~ List the the files first, sorted alphabetically, and the folders second, also sorted alphabetically. Note that some file/folder names begin with a period instead of a letter. The period should be first in the ordering. So the name ".xyz" should precede the name "abc" as it does in a directory listing.
~ Begin the description with a noun (i.e. "Folder that contains ...", "CMake configuration file, which ..." ) and end with a period.

"PREREQUISITES" SECTION ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Provide a bulleted list of items (hardware, software, tools, etc.) that must be in place before beginning the setup.

"SETUP" SECTION ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Provide a numbered list of steps that must be completed before the sample can be built and run. Setup typically includes installing dependencies and configuring the sample. If the setup is complicated with more than a few steps or has some optional parts, include H3 subheadings (### in Markdown).

"BUILD AND RUN THE SAMPLE" SECTION ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Immediately following the section heading, include the statement that references the build instructions, as shown in the Markdown template above.

After the build instructions, describe how to view the output or results when running the sample. You can use optional subsections if needed:

- The instructions for viewing output/results can be put under an H3 subheading (### in Markdown), as appropriate for the particular sample.
~ If the customer must do something, such as press a button, to see what the sample does, use the "Test the sample" H3 subheading.
~ If the customer need not do anything but observe, use the "Observe the output" H3 subheading.

"REBUILD THE SAMPLE TO ..." SECTION (optional) ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Add this section only if you want to provide a variation of the original sample. If you have more than one variation, add this section for each different variation.

~ Briefly state what the revised sample will do that is different from the original sample.
~ Describe how to modify the sample. Add paragraphs, a numbered list of steps, or subsections (under a ### heading), as appropriate. A simple modification might need only a paragraph. Something that requires more than a few steps might need subsections.
~ Add a final paragraph, step, or subsection that describes how to build/run/test the sample. You can just add a bookmark to link back to the "Build and run the sample" section if the build/run/test instructions are the same as those of the original sample.

"TROUBLESHOOTING" SECTION (optional) ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Provide details for troubleshooting common problems with this sample. Briefly describe the problem and its resolution.

~ You may want to use a numbered list of steps to describe how to resolve the problem.
~ If the problem requires detailed explanation and resolution steps or if the problem is common to many samples, provide a link to a separate file or webpage that provides the troubleshooting information.

"NEXT STEPS" SECTION ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Provide a bulleted list of references to topics or examples that allow customers to gain additional insight or a broader understanding.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ -->

<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ -->
<!-- ~~            GENERAL FORMATTING GUIDELINES AND NOTES                   ~~
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

General formatting guidelines and some notes about this README.md template are provided below. Guidelines for specific components of the Markdown are provided under GUIDELINES FOR THE MARKDOWN COMPONENTS beginning on line 145.

SOME NOTES ABOUT THE TEMPLATE ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

~ This template specifies the requirements for the README.md files that accompany the Azure Sphere samples that are available via the Samples Browser and GitHub.
~ The template contains metadata (delimited by "---") and Markdown. The metadata ensures that the Azure Sphere sample is discoverable in the Samples Browser and that all necessary files/folders are included in the ZIP file. The Markdown specifies the content to be rendered for display to the customer.
~ The metadata (YAML front matter) includes YAML comments that provide information and guidance specific to the metadata. Other comments in the template are in HTML comment tags. You should remove all comments from your README.md file.
~ Comments (YAML and HTML) are easily distinguished from metadata and Markdown if you view this template in Visual Studio Code with a theme that displays both types of comments in a distinct color. Visual Studio Code also allows the HTML comments to be collapsed.
~ You can easily view just the Markdown template and examples of the components if you open the Markdown preview.
~ You should copy this template file and rename it for your README.md. Do not change the template itself.

ALERTS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If you need alerts, don't use the ms-docs formatting; it won't work in the README.md files. Use the following formats instead. You should probably only need the first two or three.

**IMPORTANT!** followed by text on same line
**Note:** followed by text on same line
**Tip:** followed by text on same line
**CAUTION!** followed by text on same line (implies potential negative consequences)
**WARNING!** followed by text on same line (implies certain danger)

LINKS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

~ To link to pages in docs.microsoft.com, you'll need to use the entire link starting with "https:". Don't link to the review site (review.docs.microsoft.com); always link to the live docs (docs.microsoft.com).
~ Always delete "/en-us" from links to the published docs. If it's omitted, readers will get their localized versions (if any) and it will default to en-us if there isn't one.
~ Links require different formatting from links in online docs. For links to other items in the samples repo, links should be relative to the GitHub Azure Sphere Samples repo (not AppSamples on ADO). Always use relative links for items within the samples repo.

GRAPHICS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

~ If your README.md requires art, you'll need to format the links differently from the way they're formatted for docs.microsoft.com.
~ Use this format for linking to an image file: ![alt-text](/path/to/file)
~ The square brackets enclose the alternate text for the graphic, and the parentheses enclose the path.
~ Put the graphics (*.png or *.jpg) in the same folder as the README.md file; use only lowercase characters in filenames.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ -->

<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ -->
<!-- ~~                EXAMPLES OF MARKDOWN COMPONENTS                       ~~
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

An example of each Markdown component is provided below. These are just examples. You should follow the MARKDOWN TEMPLATE (begins on line 73) and read the GUIDELINES FOR THE MARKDOWN COMPONENTS (begins on line 145).

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ --><br>

––––––––––––––––      EXAMPLE: Title and Introduction      –––––––––––––––––––

```markdown
# Sample: Custom NTP high-level app

This sample application demonstrates how to configure custom NTP servers on an MT3620 device.

The sample configures the NTP servers according to your configuration in the application manifest file. The last-time-synced information is retrieved when button A is pressed. The color of the LED indicates the time-synced status.
```

–––––––––––––––––––      EXAMPLE: Libraries table      –––––––––––––––––––––––

```markdown
The sample uses the following Azure Sphere libraries.

| Library | Purpose |
|---------|---------|
| [eventloop](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invokes handlers for timer events. |
| [gpio](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-gpio/gpio-overview) | Accesses button A and LED 1 on the device. |
| [log](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) | Displays messages in the Device Output window during debugging. |
```

––––––––––––––––––––––      EXAMPLE 1: Contents      –––––––––––––––––––––––––

```markdown
## Contents

| File/folder                             | Description |
|-----------------------------------------|-------------|
| `intercore.code-workspace`              | A Visual Studio Code workspace file that allows building and debugging the RTApp and the high-level app at the same time. |
| `launch.vs.json`                        | JSON file that tells Visual Studio how to deploy and debug the application. |
| `README.md`                             | This README file. |
| `IntercoreComms_HighLevelApp`           | Folder containing the configuration files, source code files, and other files needed for the high-level application. |
| `IntercoreComms_RTApp_MT3620_BareMetal` | Folder containing the configuration files, source code files, and other files needed for the real-time capable application (RTApp). |
```

––––––––––––––––––––––      EXAMPLE 2: Contents      –––––––––––––––––––––––––

```markdown
## Contents

| File/folder           | Description |
|-----------------------|-------------|
| `app_manifest.json`   | Application manifest file, which describes the resources. |
| `CMakeLists.txt`      | CMake configuration file, which Contains the project information and is required for all builds. |
| `CMakeSettings.json`  | JSON file for configuring Visual Studio to use CMake with the correct command-line options. |
| `launch.vs.json`      | JSON file that tells Visual Studio how to deploy and debug the application. |
| `LICENSE.txt`         | The license for this sample application. |
| `main.c`              | Main C source code file. |
| `README.md`           | This README file. |
| `.vscode`             | Folder containing the JSON files that configure Visual Studio Code for building, debugging, and deploying the application. |
| `HardwareDefinitions` | Folder containing the hardware definition files for various Azure Sphere boards. |
```

––––––––––––––––––––––      EXAMPLE: Prerequisites      –––––––––––––––––––––––––

```markdown
## Prerequisites

This sample requires the following hardware:

- An [Azure Sphere development board](https://aka.ms/azurespheredevkits) that supports the [Sample Appliance](../../../HardwareDefinitions) hardware requirements.

   **Note:** By default, the sample targets the [Reference Development Board](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design, which is implemented by the Seeed Studios MT3620 Development Board. To build the sample for different Azure Sphere hardware, change the value of the TARGET_HARDWARE variable in the `CMakeLists.txt` file. For detailed instructions, see the [Hardware Definitions README](../../../HardwareDefinitions/README.md) file.

- ST LSM6DS3 accelerometer
- 2 x 10K ohm resistors
- A breadboard (recommended because this sample requires wiring from multiple sources to the same pin and the use of pull-up resistors)
- Jumper wires to connect the boards
```

––––––––––––––––––––––––––      EXAMPLE: Setup      –––––––––––––––––––––––––––––

```markdown
## Setup

1. Ensure that your Azure Sphere device is connected to your computer, and your computer is connected to the internet.
1. Even if you've performed this setup previously, ensure that you have Azure Sphere SDK version 21.01 or above. At the command prompt, run **azsphere show-version** to check. Upgrade the Azure Sphere SDK for [Windows](https://docs.microsoft.com/azure-sphere/install/install-sdk) or [Linux](https://docs.microsoft.com/azure-sphere/install/install-sdk-linux) as needed.
1. Enable application development, if you have not already done so, by entering the following line at the command prompt:

   `azsphere device enable-development`

1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repository and find the *ADC_HighLevelApp* sample in the *ADC* folder or download the zip file from the [Microsoft samples browser](https://docs.microsoft.com/samples/azure/azure-sphere-samples/adc/).

### Set up the ADC connections

1. Connect MT3620 dev board pin H2.2 (GND) to an outer terminal of the potentiometer.
1. Connect both pin 1 and pin 2 of jumper J1 to the other outer terminal of the potentiometer. This connects the MT3620 2.5 V output to the ADC VREF pin and to the potentiometer.  
1. Connect MT3620 dev board pin H2.11 (GPIO41 / ADC0) to the center terminal of the potentiometer.
```

––––––––––––––––––      EXAMPLE 1: Build and run the sample      –––––––––––––––––––

```markdown
## Build and run the sample

To build and run this sample, follow the instructions in [Build a sample application](../../../BUILD_INSTRUCTIONS.md).

To start the download, press button A on the board. The sample downloads status information for HTTP statuses 200 (success) and 400 (bad request) from the `httpstat.us` website.
```

––––––––––––––––––      EXAMPLE 2: Build and run the sample      –––––––––––––––––––

```markdown
## Build and run the sample

To build and run this sample, follow the instructions in [Build a sample application](../../../BUILD_INSTRUCTIONS.md).

### Test the sample

The output messages are displayed in the **Device Output** window during debugging.

LED1 on the MT3620 begins blinking red. Press button A on the MT3620 repeatedly to cycle through the three possible LED blink rates.
```

––––––––––––––      EXAMPLE: Rebuild the sample to {you specify}      ––––––––––––––––

```markdown
## Rebuild the sample to query a different DNS server

By default, this sample queries the _sample-service._tcp.local DNS server address. To rebuild the sample to query a different DNS server, complete the following steps.

1. Make the following changes in the sample:

    1. In the `app_manifest.json` file, change the value of the **AllowedConnections** field from `"_sample-service._tcp.local"` to the new DNS server address, such as `"_http._tcp.local"`.
    1. In `main.c`, find the code `static const char DnsServiceDiscoveryServer[] = "_sample-service._tcp.local";"` and replace `_sample-service._tcp.local` with the new DNS server address.

1. Follow the instructions in the [Build and run the sample](#build-and-run-the-sample) section of this README.
```

–––––––––––––––––––––––      EXAMPLE: Troubleshooting      –––––––––––––––––––––––––––

```markdown
## Troubleshooting

The following message in device output may indicate an out of memory issue:

`Child terminated with signal = 0x9 (SIGKILL)`

You can mitigate this problem by disabling the CURLOPT_SSL_SESSIONID_CACHE option when you create cURL handles, as shown in the following example:

`curl_easy_setopt(curlHandle, CURLOPT_SSL_SESSIONID_CACHE, 0);`
```

––––––––––––––––––––––––––      EXAMPLE: Next steps      –––––––––––––––––––––––––––––

```markdown
## Next steps

- For an overview of Azure Sphere, see [What is Azure Sphere](https://docs.microsoft.com/azure-sphere/product-overview/what-is-azure-sphere).
- To learn more about Azure Sphere application development, see [Overview of Azure Sphere applications](https://docs.microsoft.com/azure-sphere/app-development/applications-overview).
- To learn about how to use power profiles to adjust the balance between performance and energy savings, see [Set power profiles](https://docs.microsoft.com/azure-sphere/app-development/set-power-profiles).
```
