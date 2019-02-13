// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma warning disable RECS0165 // Asynchronous methods should return a Task instead of void
#pragma warning disable CS4014 // Because this call is not awaited, execution of the current method continues before the call is completed

namespace Microsoft.Azure.Sphere.Samples.WifiSetupAndDeviceControlViaBle
{
    using System;
    using System.Collections.ObjectModel;
    using System.Diagnostics;
    using System.Linq;
    using Microsoft.Azure.Sphere.Samples.WifiSetupAndDeviceControlViaBle.Contracts;
    using Windows.Devices.Enumeration;
    using Windows.Foundation;
    using Windows.UI.Popups;
    using Windows.UI.ViewManagement;
    using Windows.UI.Xaml.Controls;
    using Windows.UI.Xaml.Navigation;

    public sealed partial class MainPage : Page
    {
        private static readonly Size ExpectedSize = new Size(450, 800);

        private DeviceWatcher deviceWatcher;
        private bool isRunning;

        public MainPage()
        {
            this.InitializeComponent();

            this.NavigationCacheMode = NavigationCacheMode.Enabled;

            // Try to ensure the app displays in a mobile application form factor.
            ApplicationView.GetForCurrentView().SetPreferredMinSize(ExpectedSize);
            ApplicationView.PreferredLaunchViewSize = ExpectedSize;
            ApplicationView.PreferredLaunchWindowingMode = ApplicationViewWindowingMode.PreferredLaunchViewSize;

            this.Loaded += MainPage_Loaded;

            DeviceList.ItemsSource = Devices;
        }

        private void MainPage_Loaded(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            // Try to enforce the app displays in a mobile application form factor.
            ApplicationView.GetForCurrentView().TryResizeView(ExpectedSize);
        }

        public ObservableCollection<BleDevice> Devices { get; } = new ObservableCollection<BleDevice>();

        private void ScanButton_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            if (!isRunning)
            {
                // NOTE: This will look for ALL Bluetooth LE devices. You could potentially modify
                //       the scanning code to also check for specific services on the device if you
                //       wish to filter the list at this point.
                StartScan();
            }
            else
            {
                StopScan();
            }
        }

        private async void DeviceWatcher_Added(DeviceWatcher sender, DeviceInformation deviceInfo)
        {
            await Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
            {
                lock (this)
                {
                    // Protect against race condition if the task runs after the app stopped the deviceWatcher.
                    if (sender == deviceWatcher && !Devices.Any(d => d.Id == deviceInfo.Id))
                    {
                        Debug.WriteLine($"Found new Bluetooth LE device: '{deviceInfo.Id}'");
                        BleDevice newDevice = new BleDevice(
                            deviceInfo.Id,
                            string.IsNullOrWhiteSpace(deviceInfo.Name) ? $"<Unknown>" : deviceInfo.Name);

                        Devices.Add(newDevice);
                    }
                }
            });
        }

        private async void DeviceWatcher_Removed(DeviceWatcher sender, DeviceInformationUpdate deviceInfoUpdate)
        {
            await Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
            {
                lock (this)
                {
                    // Protect against race condition if the task runs after the app stopped the deviceWatcher.
                    if (sender == deviceWatcher && Devices.Any(d => d.Id == deviceInfoUpdate.Id))
                    {
                        Debug.WriteLine($"Bluetooth LE device no longer visible: '{deviceInfoUpdate.Id}'");
                        BleDevice foundDevice = Devices.First(d => d.Id == deviceInfoUpdate.Id);

                        Devices.Remove(foundDevice);
                    }
                }
            });
        }

        private async void DeviceWatcher_EnumerationCompleted(DeviceWatcher sender, object e)
        {
            await Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
            {
                Debug.WriteLine($"Bluetooth LE scan has completed.");

                if (Devices.Count == 0)
                {
                    MessageDialog alert = new MessageDialog("Could not find any devices.", "Alert");
                    alert.Commands.Add(new UICommand("OK"));
                    alert.ShowAsync();
                }
            });
        }

        private void DeviceList_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            // Only trigger this if an item is being selected.
            if (e.AddedItems.Count > 0)
            {
                StopScan();

                BleDevice selectedDevice = (BleDevice)e.AddedItems[0];

                this.Frame.Navigate(typeof(DevicePage), selectedDevice);
            }
        }

        private void StartScan()
        {
            // Additional properties we would like about the device.
            // Property strings are documented here https://msdn.microsoft.com/en-us/library/windows/desktop/ff521659(v=vs.85).aspx
            string[] requestedProperties = { "System.DeviceInterface.Bluetooth.ServiceGuid" };

            // Example showing paired and non-paired in a single query.
            string aqsAllBluetoothLEDevices = "(System.Devices.Aep.ProtocolId:=\"{bb7bb05e-5972-42b5-94fc-76eaa7084d49}\")";

            // Stop any running scan
            StopScan();

            deviceWatcher =
                    DeviceInformation.CreateWatcher(
                        aqsAllBluetoothLEDevices,
                        requestedProperties,
                        DeviceInformationKind.AssociationEndpoint);

            // Register event handlers before starting the watcher.
            deviceWatcher.Added += DeviceWatcher_Added;
            deviceWatcher.Removed += DeviceWatcher_Removed;
            deviceWatcher.EnumerationCompleted += DeviceWatcher_EnumerationCompleted;

            // Start over with an empty collection.
            Devices.Clear();

            // Start the watcher. Active enumeration is limited to approximately 30 seconds.
            // This limits power usage and reduces interference with other Bluetooth activities.
            Debug.WriteLine("Start listening for Bluetooth LE devices.");
            deviceWatcher.Start();

            isRunning = true;
            ScanButton.Content = "Stop scanning";
        }

        private void StopScan()
        {
            if (deviceWatcher != null)
            {
                if (deviceWatcher.Status == DeviceWatcherStatus.Started)
                {
                    // Stop the watcher.
                    Debug.WriteLine("Stop listening for Bluetooth LE devices.");
                    deviceWatcher.Stop();
                }

                // Unregister the event handlers.
                deviceWatcher.Added -= DeviceWatcher_Added;
                deviceWatcher.Removed -= DeviceWatcher_Removed;
                deviceWatcher.EnumerationCompleted -= DeviceWatcher_EnumerationCompleted;

                deviceWatcher = null;

                isRunning = false;
                ScanButton.Content = "Scan for devices";
            }
        }
    }
}
