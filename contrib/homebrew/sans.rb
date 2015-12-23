class Sans < Formula
  desc "A simple anti-spoofing DNS server"
  homepage "https://github.com/XiaoxiaoPu/sans"
  head "https://github.com/XiaoxiaoPu/sans.git"

  depends_on "autoconf" => :build
  depends_on "automake" => :build
  depends_on "libtool" => :build

  def install
    system "autoreconf", "-if"
    system "./configure", "--prefix=#{prefix}", "--sysconfdir=#{HOMEBREW_PREFIX}/etc", "--mandir=#{man}"
    system "make"
    system "make install"
  end

  test do
    system "true"
  end
end
