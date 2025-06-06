# `i2c`

I<sup>2</sup>C バスインターフェイス上のデバイスへのアクセスをまとめたモジュールです.

## 構成

ソースコード上は,
[`i2c.h`](../src/i2c.h) と
[`i2c.c`](../src/i2c.c)
で実装されています.

[`i2c.h`](../src/i2c.h) は当モジュールを使用するソースが `#include` するためのヘッダーファイルで,
当モジュールが提供する関数のプロトタイプが宣言されています.

[`i2c.c`](../src/i2c.c) は当モジュールの実装です.
以下の関数で構成されています.

| 関数名 | 役割 |
| --- | --- |
| [`i2cOpen`](#i2copen)				| I<sup>2</sup>C デバイスをオープンします. |
| [`i2cRead`](#i2cread)				| I<sup>2</sup>C バスから読み込みます. |
| [`i2cWrite`](#i2cwrite)			| I<sup>2</sup>C バスに書き込みます. |
| [`i2cRegReadByte`](#i2cregreadbyte) 		| I<sup>2</sup>C デバイスのレジスターから 1byte 読み込みます. |
| [`i2cRegWriteByte`](#i2cregwritebyte)		| I<sup>2</sup>C デバイスのレジスターに 1byte 書き込みます. |
| [`i2cRegReadWordBE`](#i2cregreadwordbe)	| I<sup>2</sup>C デバイスのレジスターから Big Endian の 1word を読み込みます. |
| [`i2cRegReadWordLE`](#i2cregreadwordle)	| I<sup>2</sup>C デバイスのレジスターから Little Endian の 1word を読み込みます. |

## `i2cOpen`

`int i2cOpen( int addr )`

| 引数 | 意味 |
| --- | --- |
| `int addr`	| I<sup>2</sup>C デバイスのスレーブアドレス |

| 返値 | オープンしたデバイスの file descriptor |
| --- | --- |

I<sup>2</sup>C デバイスをオープンします.

当モジュール使用の際には, 必ず最初に呼び出してください.
この関数内で指定された I<sup>2</sup>C スレーブアドレスのデバイスをオープンします.

大したことはしていませんが,

* 特定のスレーブアドレスに file descriptor をひも付けする

という処理が  I<sup>2</sup>C 独自のヘッダーファイルを要するので,

「アプリのソース上で直接やるより “I<sup>2</sup>C 層のソフト” として切り離した方がまとまりがよさそう」

と考えたのがこのモジュールの発端です.

なお, この「ひも付け」は,

「今後この file descriptor でアクセスが来たら、このスレーブアドレスを添えてバスに出せばいいのね」

という心構えを, `/dev/i2c-1` のデバイスドライバーに持ってもらうだけのことなので,
実在しないアドレスを指定してもエラーにはなりません.
その後実際の読み書きが行われたときにはじめて,
そのスレーブアドレス指名に応答するデバイスが居ないことが発覚する、といった次第になります.
<br>
<sup>
ダミーの読み込みでも挟んでオープン時点で判明させるのも手ですが,
それで副作用が出るデバイスも存在しないとは限らないので, 手出しは控えています.
</sup>

なので, エラー処理をきちんとする場合は,
この `i2cOpen` の戻り値だけで見切らず,
実際の読み書き処理の中にエラー判定を入れておくべきです.

<sub>
ところで, bookworm 時点でも「slave」address とか呼んでるけど,
ポリコレ的に問題視されてないのかなあ.
いずれ大々的に変更されたりなんかして.
</sub>


## `i2cRead`

`int i2cRead( int fd, void *p, int size )`

| 引数 | 意味 |
| --- | --- |
| `int fd`	| [`i2cOpen`](#i2copen) で得た file descriptor |
| `void *p`	| 読み込んだデータの格納先 |
| `int size`	| 上記格納先のバイト数 |

| 返値 | 読み込んだデータのバイト数 |
| --- | --- |

I<sup>2</sup>C バスから読み込みます.

「バスから」といってもこの関数に渡される file descriptor には指定のデバイスのスレーブアドレスがひも付けされているので,
結果的には指定のデバイスだけがバスに出してきたデータをこちらは取り込むことになります.

その手順を, I<sup>2</sup>C バス上で眺めるとこんな感じです.

![](../pics/i2cRead.png)
<br>
<sub>
S: Start condition,
P: Stop condition,
A6:A0: Slave adderess,
r/w: 0 = write / 1 = read,
ACK: Acknowledge,
D7:D0: Read data
<br>
黄色い字は Master から Slave への送信 / 青い字は Slave から Master への返信を示す.
</sub>

スレーブアドレスの指定はこちらから出力していて,
そのスレーブアドレスでご指名のデバイスからデータが上がってくるので,
それを取り込んでいます.
複数 bytes を読み込む場合は,
予定の byte 数に達するまでマスターは ACK を送って次を促し,
予定の byte 数に達したところで NACK を返して通信を終了します.

この出力と入力の織り交ざった手順が
 [`read(2)`](https://man7.org/linux/man-pages/man2/read.2.html) 一発で行われるわけです.
ありがとう, `/dev/i2c-1` のデバイスドライバー.
ありがとう, スレーブアドレスにひも付けされた file descriptor.

という感じですが,
ソフト的には単に [`read(2)`](https://man7.org/linux/man-pages/man2/read.2.html)
に丸投げした結果を無加工で返しているだけの wrapper 的な関数です.
次の [`i2cWrite`](#i2cwrite) も似たような存在なので,
「こんなんだったらわざわざ独立したモジュールを起こすまでもなかったんじゃないの?」
と心の迷いが生じないでもありませんでした.


## `i2cWrite`

`int i2cWrite( int fd, void *p, int size )`

| 引数 | 意味 |
| --- | --- |
| `int fd`	| [`i2cOpen`](#i2copen) で得た file descriptor |
| `void *p`	| 書き込むデータの格納先 |
| `int size`	| 書き込むデータのバイト数 |

| 返値 | 書き込んだデータのバイト数 |
| --- | --- |

I<sup>2</sup>C バスに書き込みます.

「バスに」といってもこの関数に渡される file descriptor には指定のデバイスのスレーブアドレスがひも付けされているので,
結果的には指定のデバイスだけがこちらがバスに出したデータを取り込むことになります.

その手順を, I<sup>2</sup>C バス上で眺めるとこんな感じです.

![](../pics/i2cWrite.png)
<br>
<sub>
S: Start condition,
P: Stop condition,
A6:A0: Slave adderess,
r/w: 0 = write / 1 = read,
ACK: Acknowledge,
D7:D0: Write data
<br>
黄色い字は Master から Slave への送信 / 青い字は Slave から Master への返信を示す.
</sub>

複数 bytes を書き込む場合は,
予定の byte 数に達するまでマスターは ACK を待って次を送り,
予定の byte 数に達したところで一方的に通信を終了します.
こちらの手順は一方的な書き込みだけなので,
[`i2cRead`](#i2cread) で感じた感謝の念も湧いてきません.

ソフト的にも単に [`write(2)`](https://man7.org/linux/man-pages/man2/write.2.html)
に丸投げした結果を無加工で返しているだけの wrapper 的な関数です.
前の [`i2cRead`](#i2cread) も似たような存在なので,
「こんなんだったらわざわざ独立したモジュールを起こすまでもなかったんじゃないの?」
と(以下略)


## i2cRegReadByte

`BYTE i2cRegReadByte( int fd, BYTE reg )`

| 引数 | 意味 |
| --- | --- |
| `int fd`	| [`i2cOpen`](#i2copen) で得た file descriptor |
| `BYTE reg`	| 読み込むレジスターの番号 |

| 返値 | 読み込んだレジスター値 |
| --- | --- |

I<sup>2</sup>C デバイスのレジスターから 1byte 読み込みます.

I<sup>2</sup>C デバイスには読み書きできる「レジスター」を持つものもあります.
<br>
( 気圧と温度を測る[BMP280](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp280-ds001.pdf) とか,
気圧と温度と湿度を測る[BME280](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf)
とか )
<br>
この関数はそうした「レジスター」から 1byte 読み出すためのものです.

一つのデバイスには複数のレジスターがあるのが普通で,
レジスター番号でどのレジスターをアクセスするのかを指定します.
つまりレジスター番号は, スレーブアドレスの下位に属するサブアドレスのようなものです.

その両者を指定して, レジスターから 1byte 読み出す手順を,
I<sup>2</sup>C バス上で眺めるとこんな感じです.

![](../pics/i2cRegRead.png)
<br>
<sub>
S: Start condition,
P: Stop condition,
A6:A0: Slave adderess,
r/w: 0 = write / 1 = read,
ACK: Acknowledge,
R7:R0: Register address,
D7:D0: Register data
<br>
黄色い字は Master から Slave への送信 / 青い字は Slave から Master への返信を示す.
</sub>

1. マスターが指定のスレーブアドレスに対してレジスター番号を書き込み.
1. マスターはいったん送信を終了.
1. マスターが同じスレーブアドレスに対して読み込み.
1. スレーブはさっき受け付けた番号のレジスターの中身をバス上に送出.
1. マスターはバス上に出てきたデータを取り込み, NACK を送ってそれ以上の送信を止める.
1. マスターが通信終了.

まあ, この流れなら
[`i2cWrite`](#i2cwrite) と [`i2cRead`](#i2cread) の合わせ技でできなくもなさそうな気もしないでもありませんが,
上記 2. と 3. の間が開くとマズいでしょう.
この間が空かないようにする
[Repeated Start Condition](https://www.i2c-bus.org/repeated-start-condition/)
というワザもあるそうで,
その場合は上の図の連続した `P` と `S` が一体化した不可分の手順となります.

で, ホントにそんなワザを繰り出しているのかどうなのか判りませんが,
可及的速やかに読み込みに着手する雰囲気が感じられる専用手順を組みました.
ガチでリアルな制御手順が入ったので,
先ほどまで感じていた「わざわざ独立したモジュールを起こすまでもなかったんじゃないの?」という心の迷いは晴れました.

<sub>
あと, 上の図の手順の締めくくりを,<br>
・必要な byte 数が返ってきたところで, Master 側が NACK を返して以降の返信を止め, STOP condition にする.<br>
・1byte 返したところで Slave 側が STOP condition にする.<br>
という 2通りの記事を見かけました.
デバイスによって異なるのかもしれませんが,
上記「専用手順」には 2bytes 読む設定もあるので,
「いつ終わるのかは Master 側が指定している」とするのがより一般的であろうとのスタンスで通しておきます.
ま, そんなこと気にしなくても, ソフトは動いてますけどね.
</sub>

## i2cRegWriteByte

`BYTE i2cRegWriteByte( int fd, BYTE reg, BYTE val )`

| 引数 | 意味 |
| --- | --- |
| `int fd`	| [`i2cOpen`](#i2copen) で得た file descriptor |
| `BYTE reg`	| 書き込むレジスターの番号 |
| `BYTE val`	| レジスターに書き込む値 |

| 返値 | 読み込んだレジスター値 |
| --- | --- |

I<sup>2</sup>C デバイスのレジスターに 1byte 書き込みます.

まあ, [`i2cRegReadByte`](#i2cregreadbyte) の逆なんですが,
I<sup>2</sup>C バス上で繰り広げられるやり取りはこんな感じです.

![](../pics/i2cRegWrite.png)

書き込む一方です.
これって [`i2cWrite`](#i2cwrite) にレジスター番号と書き込むデータが並んだ 2bytes を渡せば済む話なんじゃないの?
と思わずにはいられませんが,
[`i2cRegReadByte`](#i2cregreadbyte) に専用手順を組み込んだ手前, 対称性というか様式美というかで,
同じレベルの処理を組み込んでおきました.

ちなみに, このレジスター番号のある位置 ( スレーブアドレスのすぐ次 ) にある 1byte を,
“Control byte” と銘打って, そのビットパターンでいろいろな制御条件を渡すという芸風のデバイスもあるようです.
その場合, “Control byte” の内容を当関数の `reg` に振って, データを `val` として扱うという応用例も考えられます.
が,
当関数は 1byte のデータしかやり取りしない仕様でできているので,
それ以上のデータを扱うケースでは,
すなおに「[`i2cWrite`](#i2cwrite) に渡すデータの最初の 1byte が “Control byte”」と扱う方が面倒がありません.

<sub>
あとスレーブアドレスの 1byte 自体を “Control byte” と呼んでいる記事も見かけましたが,
そういう表現をすると話がややこしくなるので,<br>ここでは
「“Control byte” とはスレーブアドレスの隣のアレである」
というスタンスで通しておきます.
</sub>


## i2cRegReadWordBE

`WORD i2cRegReadWordBE( int fd, BYTE reg )`

| 引数 | 意味 |
| --- | --- |
| `int fd`	| [`i2cOpen`](#i2copen) で得た file descriptor |
| `BYTE reg`	| 読み込むレジスターの番号 |

| 返値 | 読み込んだレジスター値 |
| --- | --- |

I<sup>2</sup>C デバイスのレジスターから Big Endian の 1word を読み込みます.

読み込む値のサイズが 16bit になっただけで, あとは [`i2cRegReadByte`](#i2cregreadbyte) と同様です.

<sub>
i2cRegReadByte の中に組み込んである「専用手順」には,
一部入れ替えるだけで 16bit 値を扱うようにもできるのですが,
ああいうめんどくさい手順を複数構えるのも気が乗らなかったので,
i2cRegReadByte を 2回読んだ結果を Big Endian に合成するというだけで済ませました.
</sub>


## i2cRegReadWordLE

`WORD i2cRegReadWordLE( int fd, BYTE reg )`

| 引数 | 意味 |
| --- | --- |
| `int fd`	| [`i2cOpen`](#i2copen) で得た file descriptor |
| `BYTE reg`	| 読み込むレジスターの番号 |

| 返値 | 読み込んだレジスター値 |
| --- | --- |

I<sup>2</sup>C デバイスのレジスターから Littel Endian の 1word を読み込みます.

読み込む値のバイトオーダーが Little Endian になっただけで, あとは [`i2cRegReadWordBE`](#i2cregreadwordbe) と同様です.

<sub>
確か, こちらの需要が先にあって,
「Little Endian だけなのもナンだな」ということで,
使いもしない Big Endian 版も様式美として付け足したような…….
</sub>

- - -

ここでのラインナップは以上です.
なぜ「16bit のレジスター値を書く」という関数が用意されていないのかというと,
たまたまそんな需要がなかったか,
[`i2cWrite`](#i2cwrite) で済ませて, 対称性とか様式美とかのことは考えないようにしたか, のどちらかです.

