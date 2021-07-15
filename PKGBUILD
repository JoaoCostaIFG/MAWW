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
sha512sums=('424d06c17c23e4778fd8b39a6e22340ef7b9d51c6c1dc7755135b26cfb359717bebe6da2ecae917599d8913774097b13c3ed12fa7d1ce099709766bb73634228')

build() {
  cd "$pkgname-$pkgver"

  make PREFIX="/usr" maww
}

package() {
  cd "$pkgname-$pkgver"

  make DESTDIR="$pkgdir/" PREFIX="/usr" install
}
