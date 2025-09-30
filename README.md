# crosscon-uc1-2

This is a repository for client/server mTLS applications for the CROSSCON
project.

Contents:
1. [References](#references)
1. [Structure](#structure)
1. [Building for buildroot and hypervisor](#building-for-buildroot-and-hypervisor)
1. [Building legacy version for NXP demo (PC) or under yocto](#building-legacy-version-for-nxp-demo-pc-or-yocto)

## References

Reference list:
* [SSL/TLS Overview](https://www.wolfssl.com/documentation/manuals/wolfssl/appendix04.html)
* [WolfSSL SSL tutorial](https://www.wolfssl.com/documentation/manuals/wolfssl/chapter11.html)
* [WolfSSL TLS Examples](https://github.com/wolfSSL/wolfssl-examples/blob/master/tls/README.md)
* [Up to speed on mTLS architecture](https://www.securew2.com/blog/mutual-tls-mtls-authentication)

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
├── include <-- Various local source files
|   └── common <-- Common codebase for RPI and NXP solutions.
├── scripts <-- Stores scripts for various automated tasks.
└── <sources and conf. files> <-- Source files are in the repo. root for best compat. between various build systems.
```

## Available demos

This solution can be build for three scenarios:
* `TWO_RPI`: Running mutual TLS on two RPIs - In this configuration the tls client and
server will run on client and server targets respectively. Both targets will
store their private keys in a secure storage provided by `PKCS#11 TA`.
* `RPI_CBA`: This configuration extends `TWO_RPI` for context based second
factor authentication support. This configuration will leverage AI server to
decide whether to authenticate the client based on it's context.
* `NXP_PUF`: Running mutual TLS on RPI4 and NXP LPC55S69 - In this configuration,
the RPI will perform a role of a server, and the LPC platform will behave as a
client. The RPI will request the LPC for second factor authentication by
challenging the PUF running on the LPC platform.

_Note: Demo names, match those in `scripts/settings.sh` respectively._

## Settings

The `scripts/settings.sh` file is used for storing user defined settings.
Settings overview:
* `DEMO` [`TWO_RPI`/`RPI_CBA`/`NXP_PUF`] - This setting changes build
configuration, depending which demo is targeted. A full list with descriptions
can be found in "[available demos](#available-demos)" section.
* `SINGLE_TARGET` [true / false] - This setting is used to define if the keys
and binaries should be deployed on two or a single target. For "two RPI" demo,
this setting shall be set to `false`. For development purposes (to run server and
client on a single machine), or for running a client on LPC55S69 the setting
shall be set to `true`.
* `PI_SERVER_HOST` [ip addr] - An ip address of server target. In a
  `SINGLE_TARGET` mode, all certificates and binaries are deployed to server.
* `PI_CLIENT_HOST` [ip addr] - An ip address of client target. Unused in
  `SINGLE_TARGET` mode.
* `CROSSCON_REPO_PATH` [path] - A path to local
  [crosscon-demos](https://github.com/3mdeb/CROSSCON-Hypervisor-and-TEE-Isolation-Demos/)
  repository. It is a built system for RPI that will be used in the process.
* `UC1_INTEGRATION_PATH` [path] - A path to local
  [uc1-integration](https://github.com/crosscon/uc1-integration/tree/main)
  repository. It is a built system for LPC55S69. Both solutions (for each
  target) use common codebase (`./include/common`). This path will allow for
  syncing sources.

How and when to fill up these setting is explained later in this readme.

## Building

For RPIs, a custom, `buildroot` based Linux OS is being built as a part of the
[Crosscon demos](https://github.com/3mdeb/CROSSCON-Hypervisor-and-TEE-Isolation-Demos/).
The resulting linux image is then run under the Crosscon hypervisor. This
section aims to explain how to use said build system to build this solution.

### Prerequisites

First
[build and run default Crosscon demos configuration for RPI](https://github.com/3mdeb/CROSSCON-Hypervisor-and-TEE-Isolation-Demos/blob/master/rpi4-ws/README.md). You
can skip flashing and running the hypervisor image. The aim is to know the
build-system is working.

### Building the image

Here's how to build image.

#### Choose demo

Decide which [demo](#available-demos) you want to build, and set up
`DEMO` variable in `scripts/settings.sh` accordingly.

#### Sync buildroot

Update `CROSSCON_REPO_PATH` in `scripts/settings.sh` with the path to the local
`CROSSCON-Hypervisor-and-TEE-Isolation-Demos` repository.

Next, execute `buildroot: sync` vs-code task, or
`scripts/buildroot_sync_src.sh`. The script does the following:
* Syncs configuration files: `buildroot` config, `wolfssl` config, `opensc`
  config and marks the packages to rebuild if needed. For buildroot config, it
  also sets up necessary options according to specified [demo](#available-demos).
* Updates `packages/Config.in` so the app is being built by default.
* Copies this repository contents to `buildroot/package/mtls`.

Remember to run this task every time you update the sources, or make it run
automatically when saving.

#### Building and deploying manually

To build the "OS" with the applications embedded in the system, run the bellow
commands depending which demo you're running.

**For `TWO_RPI` and `NXP_PUF`**:

```bash
# Run the following inside the container
rpi4-ws/build_rpi4.sh --all --local-confs
```

**FOR `RPI_CBA`:**

```bash
./rpi4-ws/build.sh --all --local-confs --dts=./rpi4-ws/rpi4-host-linux.dts
```

The above commands will rebuild necessary components. The `--local-confs` flag
makes sure to use local configs, that were synced in a previous step.

To build hypervisor image run the following command.

**For `TWO_RPI` and `NXP_PUF`**:

```bash
# sudo is necessary here
sudo env/create_hyp_img.sh
```

**FOR `RPI_CBA`:**

```bash
# sudo is necessary here
sudo rpi4-ws/create_hyp_img.sh --config="rpi4-per-vm-vTEE"
```

To flash the image you can use `dd` command.

```bash
sudo dd if=./crosscon-demo-img.img of=<drive> bs=4M conv=fsync
```

#### Building and deploying automatically

**Note: To use automatic build and deployment, you need to deploy the solution
manually at least one time!** The reason is, for the solution to work,
the necessary libraries must be embedded into the system image. During the
automatic build and deployment, only resulting binaries are rebuilt and
transferred. The perk is the whole image does not have to be rebuilt and
re-flashed, and that the binaries can be updated on running system.

The best way to sync sources with buildroot, rebuilt the binaries and transfer
them to the targets is to use `buildroot: deploy binaries` pre-configured vscode
task. You can also fall back to executing the scripts manually.

```bash
scripts/buildroot_sync_src.sh && scripts/buildroot_deploy_bins.sh
```

_Note: You need to update target(s) ip addresses in `settings.sh`._
_This is explained later in this document._

#### Booting up

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

_Note: If building for two RPIs, repeat this step for second RPI._

#### Generating key pairs

This part describes how to generate key pair using PKCS#11 TA as secure storage.

**Note: The keys and certificates will be stored in initramfs and will disappear
after shutdown. The following steps need to be performed after each boot.**

##### Update IP addresses

First step is to request IP via DHCP on both targets.
```bash
udhcpc -i eth0
```

Second step is to update `PI_SERVER_HOST` and `PI_CLIENT_HOST` in
`scripts/settings.sh`.

If building for dual RPIs, choose one of the platforms to be the server and the
other one to be the client.

```bash
[...]
export PI_SERVER_HOST=192.168.10.29
export PI_CLIENT_HOST=192.168.10.30
[...]
```

If building for single target, update `PI_SERVER_HOST` address and set
`SINGLE_TARGET` to `true`.

```bash
[...]
export SINGLE_TARGET=true

# Target IPs
export PI_SERVER_HOST=192.168.10.38
[...]
```

Replace the values with the output of `udhcpc -i eth0`.

##### Generate certificates

For the solution to work one needs to generate private keys for both
server and client, and then generate CSRs (Certificate Signing Requests). For
the demo your PC will serve as a CA. On RPI, CSRs are generated using OpenSSL
with PKCS#11. The CSRs are transferred to the host and used for generating
certificates using the CA certificate located in `certs/ca-cert.pem`.

This works works similarly for LPC55S69, the difference is the private certs are
generated locally and then are embedded into the solution's source code. To
generate certificates for NXP platform, use `scripts/gen_and_convert_certs.sh`
script, **although you shouldn't have to do that**. The ones embedded into
the current sources should be valid.

To generate keys and certificates execute `buildroot: setup target` vscode task. Depending on `SINGLE_TARGET` settings, the task will create and deploy all
necessary files either on one or two RPIs. It'll also set up a proper date on
the targets, to avoid issues with certificate expiration. If you cannot execute
the task, you can fall back to executing the scripts manually.

```bash
scripts/buildroot_ta_key_gen.sh && scripts/buildroot_ta_cert_gen.sh
```

_Note: If you're using
[automated approach](#building-and-deploying-automatically) for building and
deploying binaries, you don't have to regenerate the certificates each time._

##### deploy binaries and generate certificates at once

The vscode task `buildroot: do all` has been set up. It rebuilds necessary
binaries, transfers them to targets, and sets up certificates. This method
can be used once, the solution has been built and deployed manually at least
once.

If not using vs-code, same result can be achieved running the following:

```bash
scripts/buildroot_sync_src.sh && \
scripts/buildroot_deploy_bins.sh && \
scripts/buildroot_ta_key_gen.sh && \
scripts/buildroot_ta_cert_gen.sh
```

## Run `TWO_RPI` demo (dual or single target)

_Note: Due to a
[known bug which causes serial connection to drop](https://github.com/crosscon/CROSSCON-Hypervisor-and-TEE-Isolation-Demos/issues/46),
it's advised to perform the following commands via ssh shell._

Run the server binary.

```bash
server-tls
```

_Note: If deploying on single platform you can use "&" at the end to run the
task in the background._

Run the client binary.

```bash
client-tls <SERVER_IP>
```

Replace `<SERVER_IP>` with the IP address of your server RPi.

Expected result:

Server:

```text
# server-tls &
# Waiting for a connection...
Server accept status: 1
Client connected successfully
SSL cipher suite is ECDHE-ECDSA-AES256-GCM-SHA384
Client: hello

Shutdown complete
Waiting for a connection...
```

Client:

```text
# client-tls <SERVER_IP>
SSL cipher suite is ECDHE-ECDSA-AES256-GCM-SHA384
Message for server: hello
Server: I hear ya fa shizzle!

Shutdown not complete
Shutdown complete
```

Do not worry if the client prints `Segmentation fault` at the end. This is a
known issue, which does not affect the process.

## Run `RPI_CBA` demo

To run `RPI_CBA` demo it is necessary to set up an AI server on development
machine. To do so, you need to update IP address and wifi settings for `cba_ta`,
as described in step 4 in
"[Building RPI image](https://github.com/crosscon/uc1-integration/tree/main/cba#building-rpi-image)" section and rebuild the solution once again. Then set
up and deploy the AI server according to
"[Building remote server](https://github.com/crosscon/uc1-integration/tree/main/cba#building-remote-server)"
section.

You might need to make a few changes so the server starts and produces logs:

```diff
diff --git a/.env b/.env
index ace12619d6fb..501769832c0b 100644
--- a/.env
+++ b/.env
@@ -8,4 +8,5 @@ SIGN_CERT_PATH=./keys/sign_cert.pem
 CSI_DATABASE_PATH=/db
 ML_MODEL_SAMPLES_PER_RECORDING=64
 ML_MODEL_CHECKPOINT_PATH=/ml/mlmodel
-ACCEPTANCE_THRESHOLD=.4
\ No newline at end of file
+ACCEPTANCE_THRESHOLD=.4
+PYTHONUNBUFFERED=1
diff --git a/docker-compose.yml b/docker-compose.yml
index d219301a79d2..2a1f9419d226 100644
--- a/docker-compose.yml
+++ b/docker-compose.yml
@@ -1,10 +1,11 @@
 services:
   remote:
-    image: crosscon-cba-remote:latest
+    image: crosscon-ra-remote:latest
     volumes:
-      - "./keys:/keys:ro"
       - "./db:/db"
       - "./e2e.pt:/ml/mlmodel:ro"
+      - "./keys:/app/keys:ro"
+      - "./verified:/verified:ro"
     env_file: ".env"
     ports:
       - 5432:5432
```

Once the server is running you can continue just as in
"[`TWO_RPI` demo](#run-two_rpi-demo-dual-or-single-target)" section.
Note that this demo takes a very long time (up to 5 minutes) to complete by
default, as it must gather CSI data two times.

## Running `NXP_PUF` demo
Here's how to run the demo for NXP platform.

### Update the path to uc1-integration and perform sync

To build the solution for NXP you'll need to sync common codebase with build
system for NXP platform. Update `UC1_INTEGRATION_PATH` in `scripts/settings.sh`
with the path to the local
[uc1-integration](https://github.com/crosscon/uc1-integration) repository.

```bash
export UC1_INTEGRATION_PATH=<path>
```

Then sync the common code base by running `nxp: sync common` vs-code task.
You can fall back to executing the `scripts/nxp_sync_common.sh` script manually.

```bash
scripts/nxp_sync_common.sh
```

If the task/script succeed you should be set to start building the client for
LPC55S69.

### Rebuild and deploy binaries

If you haven't already, set `SINGLE_TARGET` mode to true.

Rebuild and deploy the solution. If you previously built for dual RPIs,
re-flashed or rebooted the image, make sure to deploy certificates too.

### Run the application

Run the server application, and once the NXP solution has been built, attempt to
connect to the running server.


## MISC

### Buildroot: mtls config settings

The local buildroot config located at
`buildroot/config/crosscon-buildroot.config` contains a various settings
that configure `mtls` application behavior. Here they are:

```bash
BR2_PACKAGE_MTLS=y
BR2_PACKAGE_MTLS_DEBUG=y
BR2_PACKAGE_MTLS_NXP_PUF=n
BR2_PACKAGE_MTLS_RPI_CBA=n
```

The first two are self-explanatory:
 * The first one enables building the `mtls` application.
 * The second one enables debug mode with extended prints.

The other two control which [demo](#available-demos) configuration is getting
built.

#### `TWO_RPI` demo

For `TWO_RPI` demo, set variables as follows:

```bash
BR2_PACKAGE_MTLS_NXP_PUF=n
BR2_PACKAGE_MTLS_RPI_CBA=n
```

This will disable PUF and context based auth.

#### `RPI_CBA` demo

For `RPI_CBA` demo, set variables as follows:

```bash
BR2_PACKAGE_MTLS_NXP_PUF=n
BR2_PACKAGE_MTLS_RPI_CBA=y
```

This will disable PUF authentication, and enable context based auth.

#### `RPI_CBA` demo

For `NXP_PUF` demo, set variables as follows:

```bash
BR2_PACKAGE_MTLS_NXP_PUF=y
BR2_PACKAGE_MTLS_RPI_CBA=y
```

This will enable PUF second factor authentication, and disable context based
auth.

#### Running dhcp server

The script at `scripts/run_dhcp_server.sh`, will allow you to create dhcp
server on ethernet port or dhcp-server with bridged connection on internal
nic via ethernet port + USB-NIC for example. Run the script with no params to
see the usage. The script has been tested on Fedora 41.

This is useful if you want to showcase context-based auth. and need to connect
the PIs directly to the PC.
