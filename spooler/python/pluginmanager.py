import imp
import glob
import os
import sys
import inspect
import operator


MP_CONTINUE_SUCCESS     = 0
MP_CONTINUE_FAILED      = 1
MP_STOP_SUCCESS         = 2
MP_STOP_FAILED          = 3

class PluginManager:
    def __init__(self, plugindir, logger):
        self.logger = logger
        self.plugindir = plugindir
        self.plugins = []

    def loadPlugins(self, baseclass):
        self.logger.logInfo("* Loading plugins started")
        if self.plugindir is None:
            raise Exception('Invalid plugins directory')

        directory = os.path.abspath(self.plugindir)
        plugins = glob.glob(directory + "/*.py")

        self.logger.logInfo("** Checking plugins in %s" % directory)
        if not directory in sys.path:
            sys.path.insert(0, directory)

        for p in plugins:
            try:
                name = p.split("/")[-1]
                name = name.split(".py")[0]

                if name.startswith('_'): continue

                self.logger.logDebug("** Inspecting file '%s.py'" % name)

                f, file, desc = imp.find_module(name, [directory])

                plugin = __import__(name)

                for k,v in inspect.getmembers(plugin):
                    if not inspect.isclass(v): continue
                    if k.startswith('_'): continue

                    if not issubclass(v, baseclass) or v is baseclass:
                        continue

                    instance = v(self.logger)
                    self.logger.logInfo("** Adding plugin '%s'" % k) # fixme, use a plugin meta name ?
                    self.plugins.append((k, instance))

            except Exception, e:
                self.logger.logError("!-- failed to load: %s" % p)
                self.logger.logError("!-- error: %s " % e)

        self.logger.logInfo("* Loading plugins done")
        return len(self.plugins)

    def processPluginFunction(self, functionname, *args):
        plugins = []
        self.logger.logInfo("* %s processing started" % functionname)

        # get the priority of the plugin function
        for (name, i) in self.plugins:

           if hasattr(i, 'prio'+functionname):
               prio = getattr(i, 'prio'+functionname)
           else:
               prio = 9999

           plugins.append((functionname, prio, name, i))

        # sort the plugins on function name and prio
        sortedlist = sorted(plugins, lambda a, b: cmp(a[0], b[0]) or cmp(a[1], b[1]))

        retval = MP_CONTINUE_SUCCESS
        for (fname, prio, cname, i) in sortedlist:
            if (fname != functionname): continue
            try:
                self.logger.logInfo("** Plugin '%s.%s' priority (%d)" % (cname, fname, prio))
                retval = getattr(i, fname)(*args)
                if (type(retval) != type(int)):
                    self.logger.logWarn("!- Plugin '%s.%s' wrong return value expect 'int' type. fallback on default return code" % (cname, fname))
                    retval = 0
                self.logger.logInfo("** Plugin '%s.%s' returncode %d" % (cname, fname, retval))
            except NotImplementedError, e:
                #Most likely the function is not implemented
                self.logger.logDebug("!---------- %s" % e)
            except Exception, e:
                self.logger.logError("!-- error: %s " % e)

            if (retval != MP_CONTINUE_SUCCESS and retval != MP_CONTINUE_FAILED):
                break

            #fixme: do we want to break if there is another exception here?

        self.logger.logInfo("* %s processing done" % functionname)
        return retval

