%{
const char *TypeFromObject(SV* sv) { return HvNAME(SvSTASH(SvRV(sv))); }
%}