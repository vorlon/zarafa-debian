

MP_CONTINUE_SUCCESS     = 0
MP_CONTINUE_FAILED      = 1
MP_STOP_SUCCESS         = 2
MP_STOP_FAILED          = 3

# just ideas?
PROCCESSING_MARK_AS_SPAM = 1234

class IMapiDagentPlugin(object):
    prioPostConverting = 9999
    prioPreDelivery = 9999
    prioPostDelivery = 9999

    def __init__(self, logger):
        self.logger = logger

    def PostConverting(self, session, addrbook, store, folder, message):
        raise NotImplementedError('PostConverting not implemented, function ignored')

    def PreDelivery(self, session, addrbook, store, folder, message):
        raise NotImplementedError('PreDelivery not implemented, function ignored')

    def PostDelivery(self, session, addrbook, store, folder, message):
        raise NotImplementedError('PostDelivery not implemented, function ignored')

    def PreRuleProcess(self, session, addrbook, store, rulestable):
         raise NotImplementedError('PreRuleProcess not implemented, function ignored')

