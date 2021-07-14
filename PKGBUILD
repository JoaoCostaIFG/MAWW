# Maintainer: Joao Costa <joaocosta.work@posteo.net>

pkgname=maww
pkgver=1.0.0
pkgrel=4
pkgdesc="Animated backgrounds on Linux"
arch=("x86_64")
depends=("imlib2" "libx11")
license=('MIT')
source=($pkgname.pkg.tar.zst)
sha512sums=('9a7a9db62747007e9639977722b37b28a9df1689f03f332c79dd343fc1ed43dd44ea4ad7ae331dcc145792c5046591e04b8e49b86de3794e7e05d1f5c8629fdc')

build() {
  cd "$pkgname-$pkgver"

  #./configure --prefix=/usr
  make PREFIX="/usr"
}

package() {
  cd "$pkgname-$pkgver"

  make DESTDIR="$pkgdir/" PREFIX="/usr" install
}
