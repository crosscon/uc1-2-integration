# Recipe created by recipetool
#
# NOTE: LICENSE is being set to "CLOSED" to allow you to at least start building - if
# this is not accurate with respect to the licensing of the software being built (it
# will not be in most cases) you must specify the correct value before using this
# recipe for anything other than initial testing/development!
LICENSE = "CLOSED"
LIC_FILES_CHKSUM = ""

SRC_URI = "git://git@git.3mdeb.com:2222/3mdeb/crosscon-uc1-2.git;protocol=ssh;branch=master"

PV = "1.0+git"
SRCREV = "2e9b43640b06f5ea2aa705e29990da3762b6f0c1"

S = "${WORKDIR}/git"

DEPENDS:append = " wolfssl"

RDEPENDS:${PN} = " \
    wolfssl \
"

do_compile () {
	oe_runmake
}

do_install () {
	oe_runmake install 'DESTDIR=${D}'
}

