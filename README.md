# crosscon-uc1-2

This is a repository for client/server mTLS applications for the CROSSCON
project.

## References

Reference list:
* [SSL/TLS Overview](https://www.wolfssl.com/documentation/manuals/wolfssl/appendix04.html)
* [WolfSSL SSL tutorial](https://www.wolfssl.com/documentation/manuals/wolfssl/chapter11.html)
* [WolfSSL TLS Examples](https://github.com/wolfSSL/wolfssl-examples/blob/master/tls/README.md)
* [Up to speed on mTLS architecture](https://www.securew2.com/blog/mutual-tls-mtls-authentication)

## Build options

Here are the build targets:

* `make` - build default configuration.
* `make debug` - builds default configuration with debug logs enabled.
* `make install` - installs binaries and certificates onto the system.
* `make clean` - removes temporary local files.

### Additional options

Here's the list of additional options. These shall be passed with `make` command
to take effect:

* `USE_ECC_CERTS=1` - Uses ECC certificates. This option is required to connect
from NXP platform.

## Building for PC (Fedora)

Here are the steps for building `mtls` for PC (eg. Fedora).

1. Install wolfssl
    ```bash
    dir=$(pwd)
    sudo dnf install -y autoconf automake libtool
    cd ~/Downloads
    git clone https://github.com/wolfSSL/wolfssl.git
    cd wolfssl
    ./autogen.sh
    ./configure
    make
    sudo make install
    cd $dir
    ```
1. Add wolfssl to dynamic linker config
    ```bash
    echo "/usr/local/lib" | sudo tee /etc/ld.so.conf.d/wolfssl.conf
    sudo ldconfig
    ```
1. Build and install `mtls`
    ```bash
    export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig
    make
    sudo make install # Necessary to install certificates in valid directory.
    ```

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

```bash
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
1. Run `scripts/builder_sync.sh` script. This will sync the contents of this
repository to builder.
1. On builder start kas-container in shell mode.

    ```bash
    KAS_MACHINE=raspberrypi4-64 kas-container \
    --runtime-args "--volume $REMOTE_PROJECT_DIR:/workdir" \
    shell meta-zarhus/kas/common.yml:meta-zarhus/kas/cache.yml:meta-zarhus/kas/debug.yml:meta-zarhus/kas/rpi.yml
    ```

    Note: **REMOTE_PROJECT_DIR** value should be taken from `scripts/common.sh`.
1. Add application to Yocto structure

    ```bash
    devtool add mtls /workdir
    ```

1. Copy recipe `yocto/mtls_git.bb` from source files to recipes directory

    ```bash
     cp /workdir/yocto/mtls_git.bb /build/workspace/recipes/mtls/mtls_git.bb
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

1. [Flash the image to RPI](https://docs.zarhus.com/getting-started/flashing/)

### Development

Once previous step have been completed. Now you should be able to:
* develop the code on your development machine
* build and deploy applications via builder.

You can run the tasks directly from `vscode`/`vscodium`. The task dependency
has been configured, so running `rpi: deploy` should also sync source code,
rebuild the image, fetch artifacts and deploy binaries on a RPI. You can also
run these steps manually by simply calling scripts in `./scripts` directory.

## Usage

The app consists of two binaries `server-tls` and `client-tls`. To test the app
first run the server as a background process:

```bash
root@raspberrypi4-64:~# server-tls &
[1] 478
Waiting for a connection...
```

Then run the client with the following command:

```bash
root@raspberrypi4-64:~# client-tls 127.0.0.1
Client connected successfully
SSL cipher suite is TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384
SSL cipher suite is TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384
```
