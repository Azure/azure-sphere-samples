// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma warning disable CS4014 // Because this call is not awaited, execution of the current method continues before the call is completed

namespace Microsoft.Azure.Sphere.Samples.WifiSetupAndDeviceControlViaBle
{
    using System;
    using System.Collections.ObjectModel;
    using Microsoft.Azure.Sphere.Samples.WifiSetupAndDeviceControlViaBle.MessageProtocol;
    using Microsoft.Azure.Sphere.Samples.WifiSetupAndDeviceControlViaBle.MessageProtocol.Contracts;
    using Microsoft.Azure.Sphere.Samples.WifiSetupAndDeviceControlViaBle.MessageProtocol.EventArgs;
    using Windows.Devices.Bluetooth.GenericAttributeProfile;
    using Windows.UI.Popups;
    using Windows.UI.Xaml.Controls;
    using Windows.UI.Xaml.Navigation;

    public sealed partial class NetworkPage : Page
    {
        private GattDeviceService service;
        private WifiScanResultRequest requestedNetwork;

        private MessageProtocolClient wifiConfigMessageProtocolClient = new MessageProtocolClient();

        public NetworkPage()
        {
            this.InitializeComponent();

            NetworkList.ItemsSource = Networks;
        }

        public ObservableCollection<WifiScanResultRequest> Networks { get; } = new ObservableCollection<WifiScanResultRequest>();

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            if (e.Parameter is GattDeviceService && e.Parameter != null)
            {
                service = e.Parameter as GattDeviceService;
            }

            base.OnNavigatedTo(e);
        }

        private void BackButton_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            this.Frame.GoBack();
        }

        private async void ScanNetworks_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            try
            {
                ScanNetworks.IsEnabled = false;

                Networks.Clear();

                wifiConfigMessageProtocolClient.WifiNetworkScanReceived += WifiConfigMessageProtocolClient_WifiNetworkScanReceived;
                await wifiConfigMessageProtocolClient.ListenForVisibleWifiNetworksAsync(service);
            }
            catch (Exception ex)
            {
                MessageDialog exceptionAlert = new MessageDialog(ex.Message, "Alert");
                exceptionAlert.Commands.Add(new UICommand("OK"));
                await exceptionAlert.ShowAsync();

                ScanNetworks.IsEnabled = true;
                return;
            }
        }

        private async void WifiConfigMessageProtocolClient_WifiNetworkScanReceived(object sender, WifiScanRequestEventArgs e)
        {
            await Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
            {
                if (e.Network != null)
                {
                    Networks.Add(e.Network);
                }

                if (e.Index == e.NetworkCount)
                {
                    wifiConfigMessageProtocolClient.WifiNetworkScanReceived -= WifiConfigMessageProtocolClient_WifiNetworkScanReceived;

                    if (e.NetworkCount == 0)
                    {
                        MessageDialog noneFoundAlert = new MessageDialog("No visible Wi-Fi networks were found.", "Info");
                        noneFoundAlert.Commands.Add(new UICommand("OK"));
                        noneFoundAlert.ShowAsync();
                    }

                    ScanNetworks.IsEnabled = true;
                }
            });
        }

        private async void NetworkList_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            requestedNetwork = (WifiScanResultRequest)e.AddedItems[0];

            if (requestedNetwork.SecurityType == SecurityType.Unknown)
            {
                MessageDialog exceptionAlert = new MessageDialog("Cannot connect to a Wi-Fi network with an unknown security type.", "Alert");
                exceptionAlert.Commands.Add(new UICommand("OK"));
                await exceptionAlert.ShowAsync();
                return;
            }

            // Show connect button
            ConnectPanel.Visibility = Windows.UI.Xaml.Visibility.Visible;

            if (requestedNetwork.SecurityType == SecurityType.WPA2)
            {
                // Show password field
                PasswordPanel.Visibility = Windows.UI.Xaml.Visibility.Visible;
                ConnectButton.IsEnabled = false;
            }
            else
            {
                // Hide password field
                PasswordPanel.Visibility = Windows.UI.Xaml.Visibility.Collapsed;
                ConnectButton.IsEnabled = true;
            }
        }

        private async void ConnectButton_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            // Lock down the UX
            ConnectButton.IsEnabled = false;

            ScanNetworks.IsEnabled = false;
            NetworkList.IsEnabled = false;
            Psk.IsEnabled = false;

            wifiConfigMessageProtocolClient.WifiAddNetworkStatusReceived += WifiConfigMessageProtocolClient_WifiAddNetworkStatusReceived;

            try
            {
                await wifiConfigMessageProtocolClient.AddWifiNetworkAsync(
                    service,
                    requestedNetwork.Ssid,
                    requestedNetwork.SecurityType,
                    requestedNetwork.SecurityType == SecurityType.WPA2 ? Psk.Password : null);
            }
            catch (Exception ex)
            {
                MessageDialog exceptionAlert = new MessageDialog(ex.Message, "Alert");
                exceptionAlert.Commands.Add(new UICommand("OK"));
                await exceptionAlert.ShowAsync();

                ConnectButton.IsEnabled = true;

                ScanNetworks.IsEnabled = true;
                NetworkList.IsEnabled = true;
                Psk.IsEnabled = true;
            }
        }

        private async void WifiConfigMessageProtocolClient_WifiAddNetworkStatusReceived(object sender, WifiAddNetworkRequestEventArgs e)
        {
            await Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
            {
                wifiConfigMessageProtocolClient.WifiAddNetworkStatusReceived -= WifiConfigMessageProtocolClient_WifiAddNetworkStatusReceived;

                if (e.ErrorCode != (byte)WifiAddNetworkRequestErrorCode.Success)
                {
                    string errorMessage = $"Device could not set Wi-Fi network. Error code: {e.ErrorCode}";

                    if (e.ErrorCode == (byte)WifiAddNetworkRequestErrorCode.NetworkExists)
                    {
                        errorMessage = "This Wi-Fi network is already set on the device.";
                    }

                    MessageDialog exceptionAlert = new MessageDialog(errorMessage, "Alert");
                    exceptionAlert.Commands.Add(new UICommand("OK"));
                    exceptionAlert.ShowAsync();

                    // Reset to try again
                    Psk.IsEnabled = true;
                    NetworkList.IsEnabled = true;
                    ScanNetworks.IsEnabled = true;

                    ConnectButton.IsEnabled = true;
                    return;
                }

                // We're set
                MessageDialog successAlert = new MessageDialog("Wi-Fi network set successfully.", "Info");
                successAlert.Commands.Add(new UICommand("OK"));
                successAlert.ShowAsync();

                // Go back to the previous page to show status
                this.Frame.GoBack();
            });
        }

        private void Psk_PasswordChanged(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            ConnectButton.IsEnabled = Psk.Password.Length > 0;
        }
    }
}
