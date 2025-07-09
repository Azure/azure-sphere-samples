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
        public ObservableCollection<SecurityType> networkTypes { get; } = new ObservableCollection<SecurityType>();

        public NetworkPage()
        {
            this.InitializeComponent();

            NetworkList.ItemsSource = Networks;

            foreach (SecurityType secType in Enum.GetValues(typeof(SecurityType)))
            {
                networkTypes.Add(secType);
            }
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

        private void NetworkList_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            requestedNetwork = (WifiScanResultRequest)e.AddedItems[0];

            NTCombo.SelectedItem = requestedNetwork.SecurityType;
            SsidTextBox.Text = System.Text.Encoding.UTF8.GetString(requestedNetwork.Ssid);
            Psk.Password = string.Empty;

            HandleNewSecurityType(requestedNetwork.SecurityType);
        }

        // Called when use selects a new security type, either by selecting an item
        // from the network list or via the network type combo box.  Enables or disables
        // the other controls as required.
        private void HandleNewSecurityType(SecurityType secType)
        {
            switch (secType)
            {
                // Leave connect button disabled until a PSK is entered.
                case SecurityType.WPA2:
                    SsidTextBox.IsEnabled = true;
                    Psk.IsEnabled = true;
                    break;

                // Don't enable user to enter a password.
                case SecurityType.Open:
                    SsidTextBox.IsEnabled = true;
                    Psk.IsEnabled = false;
                    break;

                // Cannot connect to unknown network type.
                case SecurityType.Unknown:
                    SsidTextBox.IsEnabled = false;
                    Psk.IsEnabled = false;
                    break;

                default:
                    System.Diagnostics.Debug.Assert(false, "Unknown security type");
                    break;
            }

            SetConnectEnabled(secType);
        }

        // Called when one of the Network Type, SSID, or PSK changes.  Enables or
        // disables Targeted Scan ToggleSwitch and Connect Button.
        private void SetConnectEnabled(SecurityType secType)
        {
            bool canConnect = false;
            bool canSetPri = false;

            switch (secType)
            {
                case SecurityType.Open:
                    canConnect = !string.IsNullOrEmpty(SsidTextBox.Text);
                    canSetPri = true;
                    break;

                case SecurityType.WPA2:
                    canConnect = (!string.IsNullOrEmpty(SsidTextBox.Text)) && (!string.IsNullOrEmpty(Psk.Password));
                    canSetPri = true;
                    break;

                case SecurityType.Unknown:
                    canConnect = false;
                    canSetPri = false;
                    break;

                default:
                    System.Diagnostics.Debug.Assert(false, "Unknown security type");
                    break;
            }

            TargetedScanToggleSwitch.IsEnabled = canSetPri;
            ConnectButton.IsEnabled = canConnect;
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
                byte[] ssidBytes = System.Text.Encoding.UTF8.GetBytes(SsidTextBox.Text);
                SecurityType st = (SecurityType)NTCombo.SelectedItem;
                string psk = (st == SecurityType.WPA2) ? Psk.Password : null;
                bool targetedScan = TargetedScanToggleSwitch.IsOn;

                await wifiConfigMessageProtocolClient.AddWifiNetworkAsync(service, ssidBytes, st, psk, targetedScan);
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

        private void Page_Loaded(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            NTCombo.SelectedItem = SecurityType.Open;
        }

        private void NTCombo_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            SecurityType secType = (SecurityType)e.AddedItems[0];
            HandleNewSecurityType(secType);
        }

        private void SsidTextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            SetConnectEnabled((SecurityType)NTCombo.SelectedItem);
        }

        private void Psk_PasswordChanged(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            SetConnectEnabled((SecurityType) NTCombo.SelectedItem);
        }
    }
}
