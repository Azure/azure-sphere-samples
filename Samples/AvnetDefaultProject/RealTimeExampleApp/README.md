# Sample: Inter-core comms ADC - real-time capable app

This sample application is the partner application to the AvnetAzureSphereHacksterTTC HighLevel application.  

This application . . . 

* Opens a intercore communication path to the HighLevel application
* Listens for commands from the HighLevel application
* Responds to HighLevel commands by reading the on-board light sensor and returning the raw ADC data to the HighLevel application

# Running the application

When running A7 (HighLevel) and M4 (Realtime) applications in a development environment, you must open both applications in seperate development tools (Visual Studio, or Visual Studio Code), build/run the M4 application first, then build/run the high level application.

In a production environment, add both the HighLevel and RealTime applications to the OTA deployment.
