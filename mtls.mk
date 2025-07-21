################################################################################
# mtls Buildroot package
################################################################################

MTLS_VERSION = 1.0

# Set MTLS_SITE to the current directory (not going up a level)
MTLS_SITE = $(TOPDIR)/package/mtls
MTLS_SITE_METHOD = local
MTLS_DEPENDENCIES = wolfssl optee-client
MTLS_INSTALL_TARGET = YES

# Define the build commands
define MTLS_BUILD_CMDS
    $(MAKE) -C $(@D) \
        CC="$(TARGET_CC)" \
        WOLFSSL_INSTALL_DIR="$(STAGING_DIR)/usr" \
		LIBTEEC_INSTALL_DIR="$(STAGING_DIR)/usr" \
        CFLAGS="$(TARGET_CFLAGS) -I$(STAGING_DIR)/usr/include" \
		LIBS="$(TARGET_LDFLAGS) -L$(STAGING_DIR)/usr/lib -lm -lwolfssl -lteec"

endef

# Define install commands
define MTLS_INSTALL_TARGET_CMDS
    $(MAKE) -C $(@D) install DESTDIR=$(TARGET_DIR)
endef

# Use the generic package build method
$(eval $(generic-package))
