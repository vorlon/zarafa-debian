import MAPI
from MAPI.Util import *
from MAPI.Time import *
from MAPI.Struct import *

from plugintemplates import *
import StringIO 
from PIL import Image


class exampleBMP2PNG(IMapiDAgentPlugin):

    prioPostConverting = 10

    def PostConverting(self, session, addrbook, store, folder, message):
        props = message.GetProps([PR_HASATTACH], 0)

        if (props[0].Value == False):
            return MP_CONTINUE_SUCCESS;

        self.FindBMPAttachmentsAndConvert(message)

        return MP_CONTINUE_SUCCESS

    def FindBMPAttachmentsAndConvert(self, message, embedded=False):
        # Get the attachments
        table = message.GetAttachmentTable(MAPI_UNICODE)
        table.SetColumns([PR_ATTACH_NUM, PR_ATTACH_LONG_FILENAME, PR_ATTACH_METHOD, PR_ATTACH_MIME_TAG], 0)
        changes = False
        while(True):
            rows = table.QueryRows(10, 0)
            if (len(rows) == 0):
                break

            for row in rows:
                method = row[2].Value
                itype = row[3].Value.upper()
                if (method == ATTACH_BY_VALUE and itype.find('BMP') >=0):
                    changes = True
                    self.ConvertBMP2PNG(message, row[0].Value)
                elif (method == ATTACH_EMBEDDED_MSG):
                    embeddedmsg = message.OpenAttach(row[0].Value, IID_IMessage, 0)
                    if (self.FindBMPAttachmentsAndConvert(embeddedmsg, True) == 0):
                        changes = True

        if (embedded and changes):
            message.SaveChanges(0)

        return changes
        
    def ConvertBMP2PNG(self, message, attnum):
        attach = message.OpenAttach(attnum, IID_IAttachment, 0)
        attprops = attach.GetProps([PR_ATTACH_LONG_FILENAME, PR_DISPLAY_NAME, PR_ATTACH_FILENAME], 0)
        stream = attach.OpenProperty(PR_ATTACH_DATA_BIN, IID_IStream, 0, MAPI_MODIFY)

        datain = StringIO.StringIO(stream.Read(0xFFFFFF))
        dataout = StringIO.StringIO()

        img = Image.open(datain)
        img.save(dataout, 'PNG') 
        stream.SetSize(0)
        stream.Seek(0,0)

        stream.Write(dataout.getvalue())
        stream.Commit(0)
        props = [SPropValue(PR_ATTACH_MIME_TAG, 'image/png'), SPropValue(PR_ATTACH_EXTENSION, '.png')]
        if (attprops[0].ulPropTag == PR_ATTACH_LONG_FILENAME):
            props.append(SPropValue(PR_ATTACH_LONG_FILENAME, attprops[0].Value[0:-3] + 'png'))

        if (attprops[1].ulPropTag == PR_DISPLAY_NAME and (attprops[1].Value.upper()).find('.BMP') >=0 ):
             props.append(SPropValue(PR_DISPLAY_NAME, attprops[1].Value[0:-3] + 'png'))

        if (attprops[2].ulPropTag == PR_ATTACH_FILENAME):
            props.append(SPropValue(PR_ATTACH_FILENAME, attprops[2].Value[0:-3] + 'png'))

        attach.SetProps(props)
        attach.SaveChanges(0)

        return 0
