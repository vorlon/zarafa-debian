import MAPI
from MAPI.Util import *
from MAPI.Time import *
from MAPI.Struct import *

from plugintemplates import *

import os


class Disclaimer(IMapiSpoolerPlugin):

    disclaimerdir = '/etc/zarafa/disclaimers'
    
    def bestBody(self, message):
        tag = PR_NULL
        bodytag = PR_BODY_W # todo use flags to support PR_BODY_A
        props = message.GetProps([bodytag, PR_HTML, PR_RTF_COMPRESSED, PR_RTF_IN_SYNC], 0)

        if (props[3].ulPropTag != PR_RTF_IN_SYNC):
            return tag

        if((props[0].ulPropTag == bodytag or ( PROP_TYPE(props[0].ulPropTag) == PT_ERROR and props[0].Value == MAPI_E_NOT_ENOUGH_MEMORY) ) and 
           (PROP_TYPE(props[1].ulPropTag) == PT_ERROR and props[1].Value == MAPI_E_NOT_FOUND) and 
           (PROP_TYPE(props[2].ulPropTag) == PT_ERROR and props[2].Value == MAPI_E_NOT_FOUND)):
            tag = bodytag
        elif((props[1].ulPropTag == PR_HTML or ( PROP_TYPE(props[1].ulPropTag) == PT_ERROR and props[1].Value == MAPI_E_NOT_ENOUGH_MEMORY) ) and
             (PROP_TYPE(props[0].ulPropTag) == PT_ERROR and props[0].Value == MAPI_E_NOT_ENOUGH_MEMORY) and
             (PROP_TYPE(props[2].ulPropTag) == PT_ERROR and props[2].Value == MAPI_E_NOT_ENOUGH_MEMORY) and
             props[3].Value == False):
            tag = PR_HTML
        elif((props[2].ulPropTag == PR_RTF_COMPRESSED or ( PROP_TYPE(props[2].ulPropTag) == PT_ERROR and props[2].Value == MAPI_E_NOT_ENOUGH_MEMORY) ) and
             (PROP_TYPE(props[0].ulPropTag) == PT_ERROR and props[0].Value == MAPI_E_NOT_ENOUGH_MEMORY) and
             (PROP_TYPE(props[1].ulPropTag) == PT_ERROR and props[1].Value == MAPI_E_NOT_FOUND) and
             props[3].Value == True):
            tag = PR_RTF_COMPRESSED
            
        return tag

    def getDisclaimer(self, extension, company):
        if company == None:
            company = 'default'

        name = os.path.join(self.disclaimerdir, company + '.' + extension)

        self.logger.logDebug("*--- Open disclaimer file '%s'" % (name) )

        f = open(name)
        return f.read()

    def PreSending(self, session, addrbook, store, folder, message):

        if os.path.isdir(self.disclaimerdir) == False:
           self.logger.logWarn("!--- Disclaimer directory '%s' doesn't exists." % self.disclaimerdir)
           return MP_CONTINUE,

        company = None

        props = store.GetProps([PR_USER_ENTRYID], 0)
        if props[0].ulPropTag == PR_USER_ENTRYID:
            currentuser = session.OpenEntry(props[0].Value, None, 0)

            userprops = currentuser.GetProps([PR_EC_COMPANY_NAME], 0)
            if userprops[0].ulPropTag == PR_EC_COMPANY_NAME and len(userprops[0].Value) > 0:
                company = userprops[0].Value
                self.logger.logDebug("*--- Company name is '%s'" % (company) )


        bodytag = self.bestBody(message)
        

        self.logger.logDebug("*--- The message bestbody 0x%08X" % bodytag)
        if PROP_ID(bodytag) == PROP_ID(PR_BODY):
            disclaimer = "\r\n" + self.getDisclaimer('txt', company)

            bodystream = message.OpenProperty(PR_BODY, IID_IStream, 0, MAPI_MODIFY)
            bodystream.Seek(0, STREAM_SEEK_END)
            bodystream.Write(disclaimer)
            bodystream.Commit(0)
        elif bodytag == PR_HTML:
            disclaimer = "<br>" + self.getDisclaimer('html', company)

            stream = message.OpenProperty(PR_HTML, IID_IStream, 0, MAPI_MODIFY)
            stream.Seek(0, STREAM_SEEK_END)
            stream.Write(disclaimer)
            stream.Commit(0)
        elif bodytag == PR_RTF_COMPRESSED:
            rtf = self.getDisclaimer('rtf', company)

            stream = message.OpenProperty(PR_RTF_COMPRESSED, IID_IStream, 0, MAPI_MODIFY)
            uncompressed = WrapCompressedRTFStream(stream, MAPI_MODIFY)

            # Find end tag
            uncompressed.Seek(-5, STREAM_SEEK_END)
            data = uncompressed.Read(5)
            for i in range(4, 0, -1):
                if data[i] == '}':
                    uncompressed.Seek(i-5, STREAM_SEEK_END)
                    break

            uncompressed.Write(rtf)
            uncompressed.Commit(0)
            stream.Commit(0)
        else:
            self.logger.logWarn("!--- No Body exists")

        return MP_CONTINUE,
