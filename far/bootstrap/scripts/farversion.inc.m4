m4_include(`farversion.m4')m4_dnl
const uint32_t FAR_VERSION = m4_esyscmd(`printf "0x%04X%04X;\n" 'MAJOR` 'MINOR`')m4_dnl
const char *FAR_BUILD="MAJOR.MINOR.PATCH";
