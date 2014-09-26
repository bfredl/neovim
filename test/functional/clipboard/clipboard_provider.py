import neovim
class NvimClipboardMock(object):
    def __init__(self, vim):
        self.provides = ['clipboard']
        self.clipboards = { 
                '*': [[], 'v'],
                '+': [[], 'v']
        }

    def clipboard_get(self, reg):
        return self.clipboards[reg]

    def clipboard_set(self, lines, regtype, reg):
        self.clipboards[reg] = [lines, regtype]

if __name__ == "__main__":
    from os import environ
    from neovim import stdio_session, PluginHost, Nvim, socket_session
    nvim = Nvim.from_session(stdio_session())
    with PluginHost(nvim, preloaded=[NvimClipboardMock]) as host:
        host.run()
