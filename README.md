# crosscon-uc1-2

This is a repository for client/server mtls applications for the CROSSCON
project.

## References

Reference list:
* [SSL/TLS Overview](https://www.wolfssl.com/documentation/manuals/wolfssl/appendix04.html)
* [WolfSSL SSL tutorial](https://www.wolfssl.com/documentation/manuals/wolfssl/chapter11.html)
* [WolfSSL TLS Examples](https://github.com/wolfSSL/wolfssl-examples/blob/master/tls/README.md)

## Structure

Below is the simplified repository structure with key components described.

```text
.
├── artifacts <-- A dir that's not tracked by git for build artifacts, etc.
├── buildroot <-- Crosscon buildroot specific files.
|   ├── config <-- Various configuration files for buildroot or it's components.
│   ├── patches <-- Patches for various buildroot components.
|   |               The issue is that in CROSSCON this is not a repository, thus
|   |               can't create patches with git or push changes.
│   └── rootfs_overlay <-- Rootfs overlay, for various configurations, etc.
├── certs   <-- Pregenerated certificates
├── patches <-- Conains patches for crosscon demos repo.
├── scripts <-- Stores scripts for various automated tasks.
├── <sources and conf. files> <-- Source files are in the repo. root for best
|                                 compat. between various build systems.
└── yocto <-- Yocto specific files for building for Zarhus.
```

## Architecture

Following diagram shows mtls architecture.

![MTLS ARCH](https://www.securew2.com/wp-content/uploads/2024/01/Picture1.png)  
_Source: https://www.securew2.com/blog/mutual-tls-mtls-authentication_

## Building for Zarhus (Internal only)

This section describes how to build and deploy application for Zarhus. It is
currently for internal development only.

### Build Zarhus image for RPI-4

Follow [this guide](https://docs.zarhus.com/supported-targets/rpi4/) to build
base Zarhus image and setup RPI.

The build shall be run on
[builder](https://git.3mdeb.com/3mdeb/nixos_machine_configs). A few things to
consider:
* If you don't have access to builder, obtain it first.
* `kas-container` should be installed by default and `bmap-tools` you don't
need.
* On builder, create yourself a working directory. The name should be same as
`$USER` on your development machine.

Build the image with following command.
```bash
SHELL=/bin/bash KAS_MACHINE=raspberrypi4-64 kas-container build meta-zarhus/kas/common.yml:meta-zarhus/kas/cache.yml:meta-zarhus/kas/debug.yml:meta-zarhus/kas/rpi.yml
```

When done, your project structure should look like this
```text
[builder@builder2:/project-data/<your_user_name>/yocto]$ ls
build  meta-openembedded  meta-raspberrypi  meta-zarhus  mtls_app  poky
```

### Add application and rebuild the image

This repository contains preconfigured `vscode`/`vscodium` tasks that will
allow you to: sync your local files with builder, build the binaries, fetch the
binaries, deploy binaries on RPI. First time however, we need to manually build
whole image that will include application to avoid dependency issues. Later for
development we'll be deploying just binaries.

Steps:
1. Inspect `scripts/common.sh` file, to verify all paths and variables are properly
set. The RPI deployment machine is likely to change, so edit it accordingly.
1. Run `scripts/yocto_builder_sync.sh` script. This will sync the contents of this
repository to builder.
1. On builder start kas-container in shell mode.
    ```bash
    KAS_MACHINE=raspberrypi4-64 kas-container \
    --runtime-args "--volume $REMOTE_PROJECT_DIR:/workdir" \
    shell meta-zarhus/kas/common.yml:meta-zarhus/kas/cache.yml:meta-zarhus/kas/debug.yml:meta-zarhus/kas/rpi.yml
    ```
    Note: **REMOTE_PROJECT_DIR** value should be taken from `scripts/common.sh`.
1. Add application to yocto structure
    ```bash
    devtool add mtls /workdir
    ```
1. Edit recipe according to `yocto/mtls_git.bb`
    ```bash
    devtool modify mtls
    ```
1. Build the application to test the recipe
    ```bash
    devtool build mtls
    ```
1. Add the package to zarhus package group. Edit
    `meta-zarhus-distro/recipes-zarhus/packagegroups/packagegroup-zarhus.bb` and
    add `mtls` to `RDEPENDS:${PN}-system`.
1. Rebuild the image
    ```bash
    devtool build-image zarhus-base-image-debug
    ```
1. Flash the image to RPI

### Development

Once previous step have been completed. Now you should be able to:
* develop the code on your development machine
* build and deploy applications via builder.

You can run the tasks directly from `vscode`/`vscodium`. The task dependency
has been configured, so running `yocto: rpi: deploy` should also sync source
code, rebuild the image, fetch artifacts and deploy binaries on a Pi. You can
also run these steps manually by simply calling scripts in `./scripts`
directory.

## Building for buildroot and hypervisor

The Crosscon demos build a custom OS that runs under hypervisor and uses
buildroot in the process. This section describes how to build the application
so it is embedded into built OS image.

### Prerequisites

**FIX ME - LINKS ARE BRANCHES UNDER REVIEW, FIX LINK WHEN MERGED**

To build the app embedded into system you must first
[build and run Crosscon demos for RPI](https://github.com/3mdeb/CROSSCON-Hypervisor-and-TEE-Isolation-Demos/tree/crscn-rpi4ws-build/env).

**FIX ME - WHEN FIX IS MERGED AND FORK SYNCED, THIS CAN BE REMOVED**

**Warning!** Make sure the main repository
(CROSSCON-Hypervisor-and-TEE-Isolation-Demos) contains
[a fix for ethernet connection support](https://github.com/crosscon/CROSSCON-Hypervisor-and-TEE-Isolation-Demos/pull/10). If not, you can apply
`patches/enable_eth.patch` on default branch.

### Building the image

Here's how to build image.

#### Sync buildroot

Update `CROSSCON_REPO_PATH` in `scripts/common.sh` with the path to the cloned
`CROSSCON-Hypervisor-and-TEE-Isolation-Demos` repository.

Next, execute `buildroot: sync` vs-code task, or `scripts/buildroot_sync.sh`
script to sync contents of this repository with Crosscon's buildroot.

Remember to run this task every time you update the sources, or make it run
automatically when saving.

#### Patch buildroot components

Several buildroot components must be patched for solution to work properly. This
unfortunately be a single simple patch, as the buildroot is not tracked by
Crosscon.

##### Patch buildroot configuration

For the app to work, additional buildroot components need to be enabled, this
is the role for this step.

Copy `buildroot/config/crosscon-buildroot.config` to
`<crosscon_demos>/buildroot/build-aarch64/.config`

Note that the config might change in the future, so check for differences first.

##### Patch wolfssl config

Wolfssl build configuration file must be patched for following reasons:
1. Enable pkcs#11 support.
2. Disable ARMv8 HW acceleration which caused "illegal instruction" issues.

Use `buildroot/patches/wolfssl_config.patch` to patch wolfssl build
configuration.  
A file that needs to be patched is
`<crosscon_demos>/buildroot/package/wolfssl/wolfssl.mk`.

##### Patch opensc (pkcs11-tool)

The `opensc` needs to be patched for generating public keys with `pkcs#11-tool`.

Use `buildroot/patches/opensc.patch` to patch `opensc` build configuration.  
A file that needs to be patched is
`<crosscon_demos>/buildroot/package/opensc/opensc.mk`.

#### Building

**FIX ME - ADD STEP SELECTION TO THE SCRIPT**

To build the "OS" with the applications embedded in the system, the steps
7 to 9 from `build_rpi4-ws_demo.sh` need to be rerun, and the files must be then
copied to SD card.

#### Testing

To test if the apps work, you must now boot up the RPI with built image.

##### Booting up

Use UART to USB adapter to connect RPI to your machine and start up minicom.

```bash
minicom -D /dev/ttyUSB0 -b 115200
```

Supply power to RPI and hit any key when asked to stop u-boot from attempting
auto-boot.

```text
[...]
scanning bus xhci_pci for devices... 2 USB Device(s) found
       scanning usb for storage devices... 0 Storage Device(s) found
Hit any key to stop autoboot:  0
U-Boot>
```

_Note: If you missed the timeframe, you can spam CTRL+C many times to achieve
same result._

Boot the image by manually loading it into the memory and "jumping" to it.

```text
fatload mmc 0 0x200000 crossconhyp.bin; go 0x200000
```

##### Test if apps are working

First step is to set up the date, as it defaults to 1970 which makes the
certificates for MTLS "expired". The below command will set date to
`Mon May  5 05:05:00 UTC 2025`.

```bash
date 050505052025
```
Run the server binary in the background.

```bash
server-tls &
```

Run the client binary.

```bash
client-tls 127.0.0.1
```

Expected result.

```text
# server-tls &
# Waiting for a connection...
# client-tls 127.0.0.1
Client connected successfully
SSL cipher suite is TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384
SSL cipher suite is TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384
Message for server: All your base are belong to us
Client: All your base are belong to us

Shutdown complete
Waiting for a connection...
Server: I hear ya fa shizzle!

Shutdown not complete
Shutdown complete
```

## Generating key pairs (buildroot only)

This part describes how to generate key pair using PKCS#11 TA as secure storage.
This works only if app was built on buildroot (not for Zarhus).

### Generate key pair

First step is to request IP via dhcp on pi target. If you've got issues running
this command [check if you've got ethernet support enabled](#prerequisites).

```bash
udhcpc -i eth0
```

Second step is to update `PI_HOST` in `scripts/common.sh`.
```bash
[...]
export PI_HOST=192.168.10.29
[...]
```

Last step is to either run `buildroot: generate keys` vs-code task, or simply
execute `scripts/buildroot_ta_key_gen.sh` script. The public key will be fetched
to `artifacts/certs/` directory.
