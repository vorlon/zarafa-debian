import MAPI
from MAPI.Util import *
from MAPI.Time import *
from MAPI.Struct import *

from wraplogger import WrapLogger
from pluginmanager import PluginManager
from plugintemplates import *

#@todo how to handle spam vs black/white list?

class DAgentPluginManager:

    def __init__(self, plugindir, logger):
        self.logger = WrapLogger(logger)
        self.pluginmanager = PluginManager(plugindir, self.logger)

        self.pluginmanager.loadPlugins(IMapiDAgentPlugin)

    def PostConverting(self, session, addrbook, store, folder, message):
        return self.pluginmanager.processPluginFunction('PostConverting', session, addrbook, store, folder, message)

    def PreDelivery(self, session, addrbook, store, folder, message):
        return self.pluginmanager.processPluginFunction('PreDelivery', session, addrbook, store, folder, message)

    def PostDelivery(self, session, addrbook, store, folder, message):
        return self.pluginmanager.processPluginFunction('PostDelivery', session, addrbook, store, folder, message)

    def PreRuleProcess(self, session, addrbook, store, rulestable):
        return self.pluginmanager.processPluginFunction('PreRuleProcess', session, addrbook, store, rulestable)


