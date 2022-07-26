import pytest
from microsoft_azure_sphere_deviceapi import network
from tests.helpers import utils


@pytest.fixture(name="fix_clean_images")
def clean_images():
    """Removes all application images that are not gdb server."""
    utils.clean_images()
    return True


@pytest.fixture(name="fix_setup_networks")
def setup_networks():
    """Sets network interface to true and removes any added wifi networks before each test."""
    network.set_network_interfaces("wlan0", True)
    utils.clean_wifi_networks()
    return True


@pytest.fixture(name="fix_clean_proxies")
def clean_proxies():
    """Deletes any added network proxies before each test."""
    network.delete_network_proxy()


@pytest.fixture(name="fix_clean_certificates")
def clean_certificates():
    """Removes added certificates before each test."""
    utils.clean_certificates()
    return True
