################################################################################
#
# wolfssl
#
################################################################################

WOLFSSL_VERSION = 5.5.3
WOLFSSL_SITE = $(call github,wolfSSL,wolfssl,v$(WOLFSSL_VERSION)-stable)
WOLFSSL_INSTALL_STAGING = YES

WOLFSSL_LICENSE = GPL-2.0+
WOLFSSL_LICENSE_FILES = COPYING LICENSING
WOLFSSL_CPE_ID_VENDOR = wolfssl
WOLFSSL_CONFIG_SCRIPTS = wolfssl-config
WOLFSSL_DEPENDENCIES = host-pkgconf

# wolfssl's source code is released without a configure
# script, so we need autoreconf
WOLFSSL_AUTORECONF = YES

WOLFSSL_CONF_OPTS = --disable-examples --disable-crypttests

ifeq ($(BR2_PACKAGE_WOLFSSL_ALL),y)
WOLFSSL_CONF_OPTS += --enable-all
else
WOLFSSL_CONF_OPTS += --disable-all
endif

ifeq ($(BR2_PACKAGE_WOLFSSL_SSLV3),y)
WOLFSSL_CONF_OPTS += --enable-sslv3
else
WOLFSSL_CONF_OPTS += --disable-sslv3
endif

################################################################
# CUSTOM - make wolfssl build properly for mtls                #
# These setting ensure we don't get illegal instruction errors #
################################################################

# Enable pkcs#11
WOLFSSL_CONF_OPTS += --enable-pkcs11

# Controll ARMv8 HW accelleration manually
WOLFSSL_CONF_OPTS += --disable-armasm

$(eval $(autotools-package))
