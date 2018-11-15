// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma warning disable RECS0165 // Asynchronous methods should return a Task instead of void

namespace Microsoft.Azure.Sphere.Samples.WifiConfigViaBle
{
    using System;
    using System.Text;
    using WifiConfigViaBle.Contracts;
    using Microsoft.Azure.Sphere.Samples.WifiConfigMessageProtocol;
    using Microsoft.Azure.Sphere.Samples.WifiConfigMessageProtocol.Contracts;
    using Microsoft.Azure.Sphere.Samples.WifiConfigMessageProtocol.EventArgs;
    using Windows.Devices.Bluetooth.GenericAttributeProfile;
    using Windows.UI.Popups;
    using Windows.UI.Xaml.Controls;
    using Windows.UI.Xaml.Navigation;
    using System.Threading;

    public sealed partial class DevicePage : Page
    {
        private string deviceId;
        private GattDeviceService service;
        private Timer timer;

        private WifiConfigMessageProtocolClient wifiConfigMessageProtocolClient = new WifiConfigMessageProtocolClient();

        public DevicePage()
        {
            this.InitializeComponent();
            this.NavigationCacheMode = NavigationCacheMode.Enabled;
        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            ConnectButton.IsEnabled = true;

            if (e.NavigationMode != NavigationMode.Back)
            {
                // Reset page
                ConnectButton.Content = "Connect";
                Device.Visibility = Windows.UI.Xaml.Visibility.Collapsed;
                DeviceNoWifi.Visibility = Windows.UI.Xaml.Visibility.Collapsed;
                DeviceWifiDetails.Visibility = Windows.UI.Xaml.Visibility.Collapsed;

                if (e.Parameter is BleDevice && e.Parameter != null)
                {
                    BleDevice device = e.Parameter as BleDevice;

                    DeviceName.Text = device.Name;
                    deviceId = device.Id;
                }
            }
            else
            {
                // If we're returning from connecting to a network,
                // set up a timer that refreshes the page every 5 seconds.
                timer = new Timer(Timer_Callback, null, 0, 5000);
            }

            base.OnNavigatedTo(e);
        }

        protected override void OnNavigatingFrom(NavigatingCancelEventArgs e)
        {
            if (timer != null)
            {
                // Stop the timer
                timer.Dispose();
                timer = null;
            }

            // Remove the event handler if it is set
            wifiConfigMessageProtocolClient.WifiStatusRequestReceived -= WifiStatusRequest_RequestReceived;

            base.OnNavigatingFrom(e);
        }

        private async void Timer_Callback(object state)
        {
            await Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
            {
                // Try to connect if we're not connecting at the moment.
                if (ConnectButton.IsEnabled)
                {
                    ConnectButton_Click(this, null);
                }
            });
        }

        private void BackButton_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            this.Frame.GoBack();
        }

        private async void ConnectButton_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            try
            {
                ConnectButton.IsEnabled = false;

                // Try to get the service
                if (service == null)
                {
                    service = await WifiConfigMessageProtocolClient.GetServiceAsync(deviceId);
                }

                // Now try to get the Wi-Fi network status of the device.
                wifiConfigMessageProtocolClient.WifiStatusRequestReceived += WifiStatusRequest_RequestReceived;
                await wifiConfigMessageProtocolClient.ListenForWifiStatusAsync(service);
            }
            catch (Exception ex)
            {
                MessageDialog exceptionAlert = new MessageDialog(ex.Message, "Alert");
                exceptionAlert.Commands.Add(new UICommand("OK"));
                await exceptionAlert.ShowAsync();

                ConnectButton.IsEnabled = true;
                return;
            }
        }

        private async void WifiStatusRequest_RequestReceived(object sender, WifiStatusRequestEventArgs e)
        {
            wifiConfigMessageProtocolClient.WifiStatusRequestReceived -= WifiStatusRequest_RequestReceived;

            WifiStatusRequest wifiStatusRequest = e.Request;

            await Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
            {
                // Set UX to show we've got a device that supports the Message Protocol service
                Device.Visibility = Windows.UI.Xaml.Visibility.Visible;

                // Set further UX if we've got a network definition
                string ssid = Encoding.UTF8.GetString(wifiStatusRequest.Ssid);
                if (!string.IsNullOrWhiteSpace(ssid))
                {
                    Ssid.Text = ssid;
                    SecurityType.Text = wifiStatusRequest.SecurityType.ToString();
                    WifiStatus.Text = wifiStatusRequest.IsWifiConnected.ToString();
                    NetworkStatus.Text = wifiStatusRequest.IsInternetConnected.ToString();
                    WifiFrequency.Text = wifiStatusRequest.FrequencyMhz.ToString();
                    Bssid.Text = wifiStatusRequest.Bssid;

                    DeviceNoWifi.Visibility = Windows.UI.Xaml.Visibility.Collapsed;
                    DeviceWifiDetails.Visibility = Windows.UI.Xaml.Visibility.Visible;
                }
                else
                {
                    DeviceNoWifi.Visibility = Windows.UI.Xaml.Visibility.Visible;
                    DeviceWifiDetails.Visibility = Windows.UI.Xaml.Visibility.Collapsed;
                }

                // Update button.
                ConnectButton.Content = "Refresh";
                ConnectButton.IsEnabled = true;
            });
        }

        private void AddNetworkButton_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            this.Frame.Navigate(typeof(NetworkPage), service);
        }
    }
}
