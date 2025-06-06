# `status`

[`ssd1306`](ssd1306.md) はこんな風に使ってくださいね的なサンプルとしての, Raspberry Pi の状態表示アプリです.

こんな感じの表示が出ます.

![](../pics/status.png)

| | 左側 | 右側 |
| --- | --- | --- |
| 上段 | 現在の時刻 | Core の温度 |
| 下段 | メモリー使用量 | CPU 使用率 |

となっています.

元来 [`ssd1306`](ssd1306.md) の表示のデモなので,
[茶色いキツネが犬を飛び越えた件](https://ja.wikipedia.org/wiki/The_quick_brown_fox_jumps_over_the_lazy_dog)に言及するとかでもよかったのですが,
なんか表示が動いているものの方が面白いかなと思ったんです.
cache の動作確認とかもありましたし.

ネーミングがアレなのは, 当方の気合の不足に起因するものです.
<br>
<sup>
一応 `man status` してそんな名前のものがないことだけは確認してから決めましたが.
</sup>

## 構成

ソースコード上は,
[`status.c`](../src/status.c)
で実装されています.

デモサンプルの構成をつぶさに述べてもしょうがないような気もしますが,
以下の関数で構成されています.

| 関数名 | 役割 |
| --- | --- |
| [`main`](#main)				| main 関数 |
| [`AcceptSignal`](#acceptsignal)		| よそからの signal 受け入れ |
| [`OnSignal`](#onsignal)			| signal ハンドラ |
| [`SetTimer`](#settimer) 			| タイマーのセット |
| [`OnTimer`](#ontimer)				| タイマーハンドラ |
| [`Update`](#update)				| 表示更新 |

## `main`

`int main( void )`

当アプリの main 関数

まず [`ssdSetup`](ssd1306.md#ssdsetup) を 128 x 64 pix で初期化しています.
132 x 64 pix なコントローラー ( SH1106 ) や小さい 128 x 32 pix の表示器を接続されている場合は,
そのように引数を差し替えてください.
ジャンパーをいじってアドレスを変えている方は,
`0x3c` を `0x3e` に変更しましょう.
<br>
<sup>
( このデモアプリは 128 x 32 pix の表示器のテストも兼ねているので,
128 x 64 pix の表示器を使った場合でも上 2行だけ使い, 下 2行は空けたままになります. )
</sup>

で, [`AcceptSignal`](#acceptsignal) で, よそから飛んできた signal をキャッチするようにしています.

特に問題がなければ,

1. [`ssdFlip`](ssd1306.md#ssdflip) で画面の向きをデフォルトに戻す.
1. [`SetTimer`](#settimer) で表示更新用のタイマーを仕掛ける.

を済ませた後,
[`pause(2)`](https://man7.org/linux/man-pages/man2/pause.2.html)
を繰り返す無限ループに入ります.

その下にもっともらしく `return 0;` とか書いてありますが, そこにたどり着くことはありません.

なんか拍子抜けですが, `main` 関数がやっていることは以上です.


## `AcceptSignal`

`static void AcceptSignal( void )`

よそからの signal 受け入れ

`SIGUSR1` が飛び込んできたら [`OnSignal`](#onsignal) を呼ぶように仕掛けています.<br>
`SIGUSR2` が飛び込んできても [`OnSignal`](#onsignal) を呼ぶように仕掛けています.


## `OnSignal`

`static void OnSignal( int signum )`

signal ハンドラ

`SIGUSR1` が飛び込んできたら, 表示の輝度をデフォルト ( `127` ) に戻します.<br>
`SIGUSR2` が飛び込んできたら, 表示の輝度を最小 ( `0` ) に絞ります.

いずれも [`ssdSetDim`](ssd1306.md#ssdsetdim) で実現しています.

```
$ kill -SIGUSR2 `pidof status`
```
とかすると, 表示が暗くなることが確認できます.


## `SetTimer`

`static void SetTimer( void )`

タイマーのセット

`SIGALRM` が飛び込んできたら [`OnTimer`](#ontimer) を呼ぶように
[`sigaction(2)`](https://man7.org/linux/man-pages/man2/sigaction.2.html)
で仕掛けておきます.

その後,
[`clock_gettime(3)`](https://man7.org/linux/man-pages/man3/clock_gettime.3.html)
を `CLOCK_REALTIME` で呼んで,
現在時刻をかなり細かく把握し,
[`usleep(3)`](https://man7.org/linux/man-pages/man3/usleep.3.html)
を呼んで, 次の [秒] 単位の更新タイミングまでをかなり細かく待ちます.

で,
待ちが解けたタイミングで,
[`setitimer(3p)`](https://man7.org/linux/man-pages/man3/setitimer.3p.html)
を「1秒」で呼んで,
次の [秒] 単位の更新タイミングに肉薄するタイマーを仕掛けます.

せっかく [NTP](https://ja.wikipedia.org/wiki/Network_Time_Protocol) の効いた環境に居るので,
[秒] 単位の更新タイミングに乗るよう追い込んでみました.
この地道な努力のおかげで,
[`OnTimer`](#ontimer) が呼ばれるタイミングは電波時計並みに正確です.


## `OnTimer`

`static void OnTimer( int signo )`

タイマーハンドラ

[`Update`](#update) を呼んでいるだけです.


## `Update`

`static void Update( void )`

表示更新

ようやく真打登場です.
冒頭の表示例に出てくるような文字列は, すべてこの関数で作っています.

【時刻表示に関して】

まず,
[`clock_gettime(3)`](https://man7.org/linux/man-pages/man3/clock_gettime.3.html)
を `CLOCK_REALTIME` で呼んで,
現在時刻を得ます.
得た現在時刻の [秒] 単位を
[`localtime(3p)`](https://man7.org/linux/man-pages/man3/localtime.3p.html)
にかけて, その [秒] の部分が
「前回この関数が呼ばれたときの 1秒後」
でなければ,
[秒] 更新タイミングに乗りなおすために [`SetTimer`](#settimer) を呼びなおします.

`CLOCK_REALTIME` は
[NTP](https://ja.wikipedia.org/wiki/Network_Time_Protocol) の影響を受けるので,
たまに現在時刻もズレる可能性があるから, というのもありますが,
当アプリを 
[service](https://man7.org/linux/man-pages/man5/systemd.service.5.html)
に乗せて, しかもかなり早いトリガーで起動したりしていると,
システム起動時の [`fake-hwclock`](https://manpages.debian.org/jessie/fake-hwclock/fake-hwclock.8.en.html)
から NTP 後の時計にバトンタッチした際にもかなりズレるから, というのもあります.
この「[秒] 更新タイミングへの乗りなおし」はシステム起動後に何度か行われますが,
一度かみ合ったあとは, さっぱり行われなくなります.
たぶん次の NTP で時刻が変更されるまではかみ合ったままでいられるのでしょう.

で, [`localtime(3p)`](https://man7.org/linux/man-pages/man3/localtime.3p.html)
で得た時刻を画面の上段左側に表示します.

ここでの注意点ですが,
[`localtime(3p)`](https://man7.org/linux/man-pages/man3/localtime.3p.html)
にかけるのはいつもの
[`time(3p)`](https://man7.org/linux/man-pages/man3/time.3p.html)
の値ではいけません.
[`clock_gettime(3)`](https://man7.org/linux/man-pages/man3/clock_gettime.3.html)
で得た `struct timespec` の `.tv_sec` でなければなりません.
両者は異なる値なんです.
`clock_gettime` の方が `time` よりわずかに先行しています.
この実験結果が出たときは,
「えー! `clock_gettime` と `time` って同じクロックソースで動いてんじゃないの?」
と思わず抗議の声が出てしまいました.
昔も[そういう論議](https://www.reddit.com/r/linux_programming/comments/1gj2opt/time_and_clock_gettimeclock_realtime_disagree_by/)があって,
[結論は「同じ」だった](https://stackoverflow.com/questions/38913264/any-difference-between-clock-gettime-clock-realtime-and-time)はずですが,
少なくとも bookworm 上で観測する限り, 両者は異なる値でした.

なんか釈然としませんが,
「`clock_gettime` で追い込んだ時刻なら `clock_gettime` の言う時刻で表示するのも道理」
と割り切りました.
いつもとはやり方が異なりますが,
`time` が返す `time_t` も `clock_gettime` が返す `.tv_sec` も,
どちらも同じ [epoch 値](https://ja.wikipedia.org/wiki/UNIX時間) です.
`localtime` としては文句があるはずもありません.
実際動かしてみると, 電波時計と同じタイミングで気持ちよく表示が更新されるので, これで正しいことになります.

【各種状態の値に関して】

上記時刻表示の処理で得たシステム時刻に基づいて,
[秒]の下ひと桁が `0` になったとき, つまり<br>
「10秒に1回」<br>
に間引いた間隔で表示内容を更新しています.

この「間引き」に関して各状態値のスタンスは:

| 状態値 | スタンス |
| --- | --- |
| Core 温度	| 温度を毎秒更新するのはやり過ぎじゃない? |
| メモリー使用量 | まあ, 毎秒でも間引きでもどっちでもいいけど. |
| CPU 使用率 | いや, 間引きじゃないと困る. <sub>( 理由は後述 )</sub> |

という感じなので,
CPU 使用率の巻き添えで, 全部を間引いた間隔で更新することにしました.

アプリ起動時に「あれ? 時計しか出てないぞ?」という時間が最大 10秒近く空くことになりますが,
以降は常にすべての表示が出そろった状態になるので,
出だしの「あれ?」には目をつぶることにしています.
時刻は常に更新されているので,
状態値がいつ更新されるか判らないといったストレスは軽減されていると思います.
むしろ, 次の最新情報の到着を時計を見ながらワクワクして待つ, という楽しみ方をしていただければと.
<sub>ムリがあるなあ.</sub>

【Core 温度】

特筆すべきことは何もありません.

よくみなさんがやられているように,
`/sys/class/thermal/thermal_zone0/temp`
を覗いて, 小数点以下を四捨五入した数字を出しているだけです.

[`ssd1306`](ssd1306.md#ssd1306) の表示のデモとしては,
ASCII コード体系にない「&#x2103;」を使ってみたというのはありますが.

【メモリー使用量】

ここでは `/proc/meminfo` を覗いて,
「メモリー総量 (`MemTotal`)」 から
「利用可能メモリー (`MemAvailable`)」 を差し引いた数字を採用しています.
「kB」と単位が添えられていますが,
実際は 1,000 bytes 単位ではなく 1,024 bytes 単位 ( いわゆる [KiB] ) です.

特定のプロセスの動向を掘り下げる場合は,
`/proc/<pid>/status` から数字を拾う, とかいろいろ書き換え甲斐のあるところです.

`MemTotal - MemAvailable` の代わりに
`MemTotal - MemFree` とすると,
cache に使われた実績のある領域がじわじわ加算されていくので,
それを眺めて「ああ、生きてるなあ、こいつ。」
とおバカな楽しみ方をすることもできます.
<br>
<sup>
(「何日ぐらいで頭打ちにになるんだろう」と観察してみたことがありますが,
結論が出る前に再起動してしまっていて, 見切ったことはありません. )
</sup>

蛇足ですが,
元々この欄には CPU 周波数を表示する処理が入っていました.
が, 当方の使い方では CPU が本気を出すことはなく,
常に `600MHz` と表示されていました.
これだと面白くないので, 「動く数字」としてこのメモリー使用量に差し替えた経緯があります.

【CPU 使用率】

数字の取得方法自体はそんなに込み入った話ではありません.
よくみなさんがやられているように,
`/proc/stat` から読み込んだ値を加工すればいいだけの話です.

`/proc/stat` に上げられているのはシステム起動時からの累積値なので,
適当なサンプリング間隔で拾った値の前後の差分から比率を割り出します.

で, 問題なのはそのサンプリング間隔です.
さしあたって 1[秒] 間隔でサンプリングしてみたところ,
あまり CPU が働いているように見えません.

`0.0%`

という数字が目立ちます.

理論上は,
OS というソフトが稼働している以上,
「CPU 使用率 0%」はありえないことだと考えます.
<sup>
<br>
Windows&reg;11 なんて OS しか動いていないのに同じ 4core なら 常に 7～8% の CPU 食ってますけどね.
( Task Manager 申告値 )
</sup>
<br>
それに,
[`top(1)`](https://man7.org/linux/man-pages/man1/top.1.html)
がそんな極端な数字を挙げているのを見たことがありません.
この環境で `top` を起動してみると

「99.8% がアイドルで、あとユーザーとシステムがちょっとずつ。」

と実にそれっぽい, 見覚えのある言い分で現状を教えてくれます.
「そうだよなあ。そんなもんだよなあ。それに比べてウチのアプリは……」
と OLED 上の表示を見返してみると……,
あれ? `top` の「ちょっとずつ」と似たような数字が出てる!
さっきまでの `0.0%` は何だったのかと再び `top` の表示を詳しく見てみると,
「ちょっと」を稼いでいるのは `top` 自身でした.
で, `top` を止めてみると, OLED 上の表示値は `0.0%` に戻りました.

CPU 使用率を計るコマンドの代表格である `top` 自身が割と CPU を食っているとは…….
<sub>( [観察者効果](https://ja.wikipedia.org/wiki/観察者効果) )</sub>

`top` って仕様を凝り過ぎなんでしょう.
個々のプロセスの状態とかいいから, ただ全体的な CPU の使用率が知りたいだけなんだけど……
という場合には向きません.
いや, デフォルトの動作が盛り過ぎなのであって,
極限まで軽くするオプションでもあれば……
と, ここで思いついたのが「特定のプロセスだけ見る」というオプションです.
で, そのオプション `-p <pid>` を使って

```
$ top -p `pidof status`
```

とやってみたところ,

「100.0% がアイドルで、あとユーザーとシステムは 0.0%」

と言い分が変わりました.

やれやれそういうことか, と「`top` 問題」は片付きました.

が,
今度は実績と信頼の `top` までもが

「CPU 使用率は 0.0%」

とか言い始めたわけです.
本格的な見分はこれから始まるのです.

で, どこから始めたのかというと,
[`top` もウチのアプリ同様 `/proc/stat` から読み込んだ値に基づいている](https://stackoverflow.com/questions/17134377/top-vs-proc-pid-stat-which-is-better-for-cpu-usage-calculation)そうなので,
問題の根源である `/proc/stat` がどう動いているのかを見てみることにしました.
とりあえず 1[秒] 間隔で数字の動きを見てみると……

動いていません. アイドル以外の項目では 1[秒] 前と同じ数字が並んでいます. 差分は `0` です. なら `0%` で正解です.

ここで `top` の表示の更新がデフォルトで 3[秒] おきだったことを思い出し,
長い時間間隔を取れば数字の動きもつかみやすいかもしれない, とやってみると……

動いていません. やはり何か根本的に……あ! 動いた!

どうも間隔が長いほど数字の動きが捉まりやすい気がしてきました.
で, 徐々に間隔を伸ばして「少なくとも 1項目の数字が確実に動く」ところを探しに行くと……

30[秒] ぐらいは必要そうに見えました.

でも, 30[秒]間 OS が何もしていない ( CPU 時間を消費していない ) ということがあるとも思えません.
それなのに全部が `0` で一つも `1` にならないとは……,
いや待て. その `1` ってどんな単位の数字なんだ?
と調べてみると, 定義値 `USER_HZ` の逆数のようで, 一般的に 10[ms] が 1[tick] という設定だそうです.

つまり, 30[秒] ぐらいは空けないと 10[ms] のタイムスライス 1コ分の時間も満たされない
ということなのでしょう.
<br>
<sup>
なんて軽い OS なんだ!
すごいぞ 旧姓 Raspbian な Raspberry Pi OS の最新版 bookworm である Debian! ( 名前多すぎ )
</sup>
<br>
まあ, それはともかく,

こんなに低い水準の CPU 使用率の環境もあるんだということが課題ですね.
しかし 30[秒] も表示が更新されないと人間の目から見てて「動いてんのか? こいつ」感がぬぐえません.

そこで妥協案として,

* たまに `0%` と出てしまうのはしょうがないとして, 10[秒] 粘った上での数字を出す.
* `0.0%` と出るのを極力避けるために, 小数点以下 2位まで掘り下げた数字で出す.

ということにしました.
当ページ冒頭にある表示例の `0.03%` は, そんな調整の結果です.
