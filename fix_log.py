import re
with open('src/rhi/gles/GLESRHI.cpp', 'r') as f:
    data = f.read()

# Fix mock LOG macros to ignore var args gracefully or print simply.
data = re.sub(r'#define LOGI\(\.\.\.\) std::cout << __VA_ARGS__ << std::endl', '#define LOGI(...) do {} while(0)', data)
data = re.sub(r'#define LOGE\(\.\.\.\) std::cerr << __VA_ARGS__ << std::endl', '#define LOGE(...) do {} while(0)', data)

with open('src/rhi/gles/GLESRHI.cpp', 'w') as f:
    f.write(data)
