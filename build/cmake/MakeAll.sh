echo "Make Win64:"
make DEBUG=n TARGET=WIN64
echo "Make Linux:"
make DEBUG=n TARGET=UNIX

echo "Make Android v7a (32 bit):"
make DEBUG=n TARGET=ANDROID

echo "Make Android v8a (64 bit):"
make DEBUG=n TARGET=ANDROIDAARCH64

echo "Make Android x64:"
make DEBUG=n TARGET=ANDROIDX64

echo "Make Android x86:"
make DEBUG=n TARGET=ANDROID86

