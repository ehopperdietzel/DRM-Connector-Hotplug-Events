/*********************************************************
 ** Example for listening to DRM connector hotplug events
 **
 ** Autor: Eduardo Hopperdietzel
 *********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <libudev.h>
#include <sys/poll.h>
#include <xf86drmMode.h>
#include <errno.h>
#include <string.h>

udev *udev;
udev_device *dev;
udev_monitor *monitor;
udev_list_entry *entry;
int fd;

/* List connected connectors */
void printConnectors()
{
    drmModeRes *resources;
    drmModeConnector *connector;

    resources = drmModeGetResources(fd);

    if (!resources)
    {
        printf("drmModeGetResources failed: %s\n", strerror(errno));
        exit(1);
    }

    for(int i = 0; i < resources->count_connectors; i++)
    {
        connector = drmModeGetConnector(fd, resources->connectors[i]);

        if(connector->connection == DRM_MODE_CONNECTED)
            printf("\tConnector[%i]:\tCONNECTED\n",connector->connector_id);
        else
            printf("\tConnector[%i]:\tDISCONNECTED\n",connector->connector_id);

        drmModeFreeConnector(connector);
    }

    drmModeFreeResources(resources);
}

/* Listen to hotplug events */
void events()
{
    pollfd fds;
    fds.events = POLLIN;
    fds.fd = udev_monitor_get_fd(monitor);

    while(true)
    {
        poll(&fds,1,-1);

        udev_device* monitorDev = udev_monitor_receive_device(monitor);

        if(monitorDev)
        {
            if(udev_device_get_devnum(dev) == udev_device_get_devnum(monitorDev))
            {
                printf("HOTPLUG EVENT.\n");
                printConnectors();
            }
        }

        udev_device_unref(monitorDev);
    }
}

int main(int argc, char *argv[])
{
    udev = udev_new();

    if (!udev)
    {
        printf("Can't create udev.\n");
        exit(1);
    }

    udev_enumerate *enumerate;
    udev_list_entry *devices;

    enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "drm");
    udev_enumerate_add_match_property(enumerate, "DEVTYPE", "drm_minor");
    udev_enumerate_scan_devices(enumerate);
    devices = udev_enumerate_get_list_entry(enumerate);

    // Find the first useful device (GPU)
    udev_list_entry_foreach(entry, devices)
    {
        // Path where device is mounted
        const char *path = udev_list_entry_get_name(entry);

        // Get the device
        dev = udev_device_new_from_syspath(udev, path);

        // Get the device name (Ej:/dev/dri/card0)
        const char *devName = udev_device_get_property_value(dev, "DEVNAME");

        drmModeRes *resources;

        fd = open(devName, O_RDWR);

        if (fd < 0)
        {
            printf("Could not open drm device\n");
            udev_device_unref(dev);
            dev = nullptr;
            continue;
        }

        monitor = udev_monitor_new_from_netlink(udev,"udev");
        udev_monitor_filter_add_match_subsystem_devtype(monitor, "drm", "drm_minor");

        udev_list_entry *tags = udev_device_get_tags_list_entry(dev);

        // Add tag filters to the monitor
        udev_list_entry_foreach(entry, tags)
        {
            const char *tag = udev_list_entry_get_name(entry);
            udev_monitor_filter_add_match_tag(monitor,tag);
        }

        udev_monitor_enable_receiving(monitor);

        printf("Connectors:\n");

        printConnectors();

        // Listen to hotplug events
        events();

        exit(0);
    }

    printf("No devices found.\n");

    return 0;
}
