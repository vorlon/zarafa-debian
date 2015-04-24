#!/usr/bin/env python
import fcntl
import os
import traceback

import zarafa

def cmd(c):
    print c
    assert os.system(c) == 0

def main():
    print 'zarafa-search-xapian-compact started'
    server = zarafa.Server()
    index_path = zarafa.Config(None, 'search').get('index_path')
    for store in server.stores():
        try:
            dbpath = os.path.join(index_path, server.guid+'-'+store.guid)
            print 'compact:', dbpath
            if os.path.isdir(dbpath):
                with open('%s.lock' % dbpath, 'w') as lockfile: # do not index at the same time
                    fcntl.flock(lockfile.fileno(), fcntl.LOCK_EX)
                    cmd('rm -rf %s.compact' % dbpath)
                    cmd('xapian-compact %s %s.compact' % (dbpath, dbpath))
                    cmd('rm -rf %s.old' % dbpath)
                    cmd('mv %s %s.old' % (dbpath, dbpath))
                    cmd('mv %s.compact %s' % (dbpath, dbpath))
                cmd('rm -rf %s.old' % dbpath)
            else:
                print 'WARNING: does not (yet) exist (new user?)'
            print
        except Exception, e:
            print 'ERROR'
            traceback.print_exc(e)
    print 'zarafa-search-xapian-compact done'

if __name__ == '__main__':
    main()
