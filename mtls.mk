################################################################################
# mtls Buildroot package
################################################################################

MTLS_VERSION = 1.0

# Set MTLS_SITE to the current directory (not going up a level)
MTLS_SITE = $(TOPDIR)/package/mtls
MTLS_SITE_METHOD = local
MTLS_DEPENDENCIES = wolfssl optee-client
MTLS_INSTALL_TARGET = YES

# Handle debug
ifeq ($(BR2_PACKAGE_MTLS_DEBUG),y)
MTLS_BUILD_TARGET = debug
MTLS_EXTRA_CFLAGS = -g -DDEBUG
else
MTLS_BUILD_TARGET = all
MTLS_EXTRA_CFLAGS =
endif

# Handle NXP_PUF - build server for NXP PUF (UC1.1) demo
ifeq ($(BR2_PACKAGE_MTLS_NXP_PUF),y)
MTLS_EXTRA_CFLAGS += -DNXP_PUF
endif

# Handle RPI_CBA - build server for RPI CBA (UC1.2) demo
ifeq ($(BR2_PACKAGE_MTLS_RPI_CBA),y)
MTLS_EXTRA_CFLAGS += -DRPI_CBA
endif

# Define the build commands
define MTLS_BUILD_CMDS
    $(MAKE) -C $(@D) $(MTLS_BUILD_TARGET) \
        CC="$(TARGET_CC)" \
        WOLFSSL_INSTALL_DIR="$(STAGING_DIR)/usr" \
        LIBTEEC_INSTALL_DIR="$(STAGING_DIR)/usr" \
        CFLAGS+="$(TARGET_CFLAGS) $(MTLS_EXTRA_CFLAGS) -I$(STAGING_DIR)/usr/include" \
        LIBS+="$(TARGET_LDFLAGS) -L$(STAGING_DIR)/usr/lib -lm -lwolfssl -lteec"
endef

# Define install commands
define MTLS_INSTALL_TARGET_CMDS
    $(MAKE) -C $(@D) install DESTDIR=$(TARGET_DIR)
endef

# Use the generic package build method
$(eval $(generic-package))
