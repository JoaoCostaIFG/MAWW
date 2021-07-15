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
sha512sums=('1241f5e21c4e3cee00b51155cfd9744a4cf60d1748f04d56d5f1b03df69a4f33923337217980da673665136b33c085b8bb858b248b58ccfac572504fdf6d2943')

build() {
  cd "$pkgname-$pkgver"

  make PREFIX="/usr" maww
}

package() {
  cd "$pkgname-$pkgver"

  make DESTDIR="$pkgdir/" PREFIX="/usr" install
}
