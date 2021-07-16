# Maintainer: JoaoCostaIFG <joaocosta.work@posteo.net>

pkgname=maww
pkgver=1.0.0
pkgrel=1
pkgdesc="Animated backgrounds on Linux"
arch=("x86_64")
url="https://github.com/JoaoCostaIFG/MAWW"
license=('MIT')
depends=("imlib2" "libx11")
source=(${pkgname}-${pkgver}.pkg.tar.zst::https://github.com/JoaoCostaIFG/MAWW/releases/download/v${pkgver}/${pkgname}-${pkgver}.pkg.tar.zst)
sha512sums=('a5946705d85c8d207e8aedb918727020f5b0492798d1ebb749c2d7c941a0db3518f32726f99f4d8ccd682e023a2bf799f78fd178b3a8f54da440b64add8d53f1')

build() {
  cd "$pkgname-$pkgver"

  make PREFIX="/usr" maww
}

package() {
  cd "$pkgname-$pkgver"

  make DESTDIR="$pkgdir/" PREFIX="/usr" install
}
