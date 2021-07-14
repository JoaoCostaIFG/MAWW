# Maintainer: JoaoCostaIFG <joaocosta.work@posteo.net>

pkgname=maww
pkgver=1.1.0
pkgrel=1
pkgdesc="Animated backgrounds on Linux"
arch=("x86_64")
url="https://github.com/JoaoCostaIFG/MAWW"
license=('MIT')
depends=("imlib2" "libx11")
source=(${pkgname}-${pkgver}.pkg.tar.zst)
sha512sums=('209981e9d086a1ab8024e65a4a6aff1b6d4e91f65efbd8e2750ccc308a7b9975a44905d295144c755a9005d394cd3b2b52282bc0fd04294ac6a69745267cc524')

build() {
  cd "$pkgname-$pkgver"

  make PREFIX="/usr" maww
}

package() {
  cd "$pkgname-$pkgver"

  make DESTDIR="$pkgdir/" PREFIX="/usr" install
}
