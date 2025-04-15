# Recipe created by recipetool
# This is the basis of a recipe and may need further editing in order to be fully functional.
# (Feel free to remove these comments when editing.)

# Unable to find any files that looked like license statements. Check the accompanying
# documentation and source headers and set LICENSE and LIC_FILES_CHKSUM accordingly.
#
# NOTE: LICENSE is being set to "CLOSED" to allow you to at least start building - if
# this is not accurate with respect to the licensing of the software being built (it
# will not be in most cases) you must specify the correct value before using this
# recipe for anything other than initial testing/development!
LICENSE = "CLOSED"
LIC_FILES_CHKSUM = ""

SRC_URI = "git://git@git.3mdeb.com:2222/3mdeb/crosscon-uc1-2.git;protocol=ssh;branch=master"

# Modify these as desired
PV = "1.0+git"
SRCREV = "2e9b43640b06f5ea2aa705e29990da3762b6f0c1"

S = "${WORKDIR}/git"

DEPENDS:append = " wolfssl"

RDEPENDS:${PN} = " \
    wolfssl \
"

do_configure () {
	# Specify any needed configure commands here
	:
}

do_compile () {
	# You will almost certainly need to add additional arguments here
	oe_runmake
}

do_install () {
	# This is a guess; additional arguments may be required
	oe_runmake install 'DESTDIR=${D}'
}

