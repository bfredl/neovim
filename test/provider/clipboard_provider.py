import neovim
class NvimClipboardMock(object):
    def __init__(self, vim):
        self.provides = ['clipboard']
        self.clipboards = { 
                '*': [[], 'v'],
                '+': [[], 'v']
        }

    def clipboard_get(self, reg):
        if reg == '"':
            reg = "*"
        return self.clipboards[reg]

    def clipboard_set(self, lines, regtype, reg):
        if reg == '"':
            reg = "*"
        self.clipboards[reg] = [lines, regtype]

if __name__ == "__main__":
    from os import environ
    from neovim.plugin_host import PluginHost
    nvim = neovim.connect()
    with PluginHost(nvim, discovered_plugins=[NvimClipboardMock]) as host:
        host.run()
