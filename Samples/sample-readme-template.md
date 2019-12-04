---
page_type: sample
languages:
  - c
products:
  - azure
  - sphere
name: HTTPS_Curl_Easy #name of the sample, to be displayed on the sample card on the samples browser landing page and in the browser title bar for the sample page. Ideally this should be short and to the point. The content should be plain-text (no Markdown or HTML). Default is first H1 in the README
urlFragment: "HTTPS/HTTPS_Curl_Easy" # The URL fragment of the published sample. The full URL will always be https://docs.microsoft.com/{locale}/samples/Azure/azure-sphere-samples/Samples/{urlFragment}. While this metadata node is not required, our recommendation is to always have this value set - this will ensure that the page link is immutable, regardless of sample README.md changes. [TODO: PO  to check that the beginning of full URL will be picked up by tools]
extendedZipContent: A list of files or folders that need to be included. Each element in extendedZipContent has to have two properties: path, defining the relative path of the content (from the current folder) that needs to be included in the ZIP, and target, which defines the folder within the ZIP file in which the content should be placed.
---

# Sample: Name of sample

![License](https://img.shields.io/badge/license-MIT-green.svg)

<See https://review.docs.microsoft.com/en-us/help/onboard/admin/samples/concepts/readme-template?branch=master> for details about the Microsfot Samples template>

<Introductory sentence that describes what the sample does. > This sample C application demonstrates how to use cURL with Azure Sphere over a secure HTTPS connection. <Link to docs, if available>

<If you need alerts, don't use the ms-docs formatting; it doesn't work here. Use these instead. You should probably only need the first two or three.

**IMPORTANT!** followed by text on same line
**Note:** followed by text on same line
**Tip:**  followed by text on same line
**CAUTION!** followed by text on same line (implies potential negative consequences)
**WARNING!** followed by text on same line (implies certain danger)
>

<Always delete "/en-us" from links to the published docs. If it's omitted, readers will get their localized versions (if any) and it will default to en-us if there isn't one.>

<Brief description of the sample. Detail should appear in the Key Concepts section later>
The sample periodically downloads the index web page at example.com, by using cURL over a secure HTTPS connection.
It uses the cURL "easy" API, which is a synchronous (blocking) API.

The sample uses the following Azure Sphere libraries.

|Library   |Purpose  |
|---------|---------|
|log     |  Displays messages in the Visual Studio Device Output window during debugging  |
|storage    | Gets the path to the certificate file that is used to authenticate the server      |
|libcurl | Configures the transfer and downloads the web page |

## Requirements

<List the software and hardware required to build and run the sample. >

List of versions, hardware, software, etc. required to build this solution, if different from the minimums described in the docs. If lists are long, suggest separating hardware from software.

    - Windows (if different from standard Minbar)
    - Visual Studio/VSCode version (if different from standard Minbar)
    - Azure Sphere SDK version xxx
    - Hardware? (list any specific hardware requirements)
    - Any other software, special tools, etc.?

    Be sure to list the supported boards (i.e. hardware form factors) for specific samples along with the hardware changes that should be made (e.g connecting pins and resistors on a dev board).

## Contents

| File/folder | Description |
|-------------|-------------|
| `src`       | Sample source code. |
| `.gitignore` | Define what to ignore at commit time. |
| `CHANGELOG.md` | List of changes to the sample. |
| `CONTRIBUTING.md` | Guidelines for contributing to the sample. |
| `README.md` | This README file. |
| `LICENSE`   | The license for the sample. |

## Prerequisites

- Download the [Azure Cosmos DB Emulator](https://docs.microsoft.com/azure/cosmos-db/local-emulator). The emulator is currently only supported on Windows.
- Install [Visual Studio Code](https://code.visualstudio.com/Download) for your platform.
- Install the Don Jayamanne's [Python Extension](https://marketplace.visualstudio.com/items?itemName=donjayamanne.python)

## Setup

1. Clone or download this sample repository.
3. Connect your Azure Spoghe
    ```bash
    pip install -r .\requirements.txt
    ```
4. Open the sample folder in Visual Studio Code or your IDE of choice.


## Build and run the sample

1. <Any setup steps, e.g....>Set up your Azure Sphere device and development environment as described in the [Azure Sphere documentation]
1. Clone the [CurlEasyHttps sample](https://github.com/Azure/azure-sphere-samples/tree/master/Samples/CurlEasyHttps) from the Azure Sphere Samples repo on Github.
1. Connect your Azure Sphere device to your PC by USB.
1. Enable Wi-Fi on your Azure Sphere device.
1. Open an Azure Sphere Developer Command Prompt and enable application development on your device if you have not already done so:
   `azsphere device prep-debug`
1. In Visual Studio, open CurlEasyHttps.sln and press F5 to compile and build the solution and load it onto the device for debugging.


## Key concepts

Let's take a quick review of what's happening in the app. Open the `app.py` file under the root directory and you find that these lines of code create the Azure Cosmos DB connection. The following code uses the connection string for the local Azure Cosmos DB Emulator. The password needs to be split up as seen below to accommodate for the forward slashes that cannot be parsed otherwise.

* Initialize the MongoDB client, retrieve the database, and authenticate.

    ```python
    client = MongoClient("mongodb://127.0.0.1:10250/?ssl=true") #host uri
    db = client.test    #Select the database
    db.authenticate(name="localhost",password='C2y6yDjf5' + r'/R' + '+ob0N8A7Cgv30VRDJIWEHLM+4QDU5DE2nQ9nDuVTqobD4b8mGGyPMbIZnqyMsEcaGQy67XIw' + r'/Jw==')
    ```

* Retrieve the collection or create it if it does not already exist.

    ```python
    todos = db.todo #Select the collection
    ```

* Create the app

    ```Python
    app = Flask(__name__)
    title = "TODO with Flask"
    heading = "ToDo Reminder"
    ```
    
## Next steps

--Add info about follow-on steps a user can do to change simple things around the sample

You can learn more about Azure Sphere on the [official documentation site](https://docs.microsoft.com/azure-sphere).

## License
For details on license, see LICENSE.txt in this directory.

## Code of Conduct
This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).