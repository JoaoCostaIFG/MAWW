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
sha512sums=('464a0fa845bfcc725c8751a8c89c6f7b45704329b7cd0021d0819206c2637c24e019208efc9acb4f66cfeda489620307512e2de54e90c6f090c90d9b65a18b43')

build() {
  cd "$pkgname-$pkgver"

  make PREFIX="/usr" maww
}

package() {
  cd "$pkgname-$pkgver"

  make DESTDIR="$pkgdir/" PREFIX="/usr" install
}
