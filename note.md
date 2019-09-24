# 3回課題
upstream https://qiita.com/xtetsuji/items/555a1ef19ed21ee42873
https://ja.cppreference.com/w/c/language/operator_precedence
これを参考に、関係演算子は加算より低めに設定する。
https://llvm.org/doxygen/classllvm_1_1CmpInst.html#a283f9a5d4d843d20c40bb4d3e364bb05
ICmpは比較だけで、どのタイプの比較なのかを書く必要がある。
https://llvm.org/doxygen/classllvm_1_1IRBuilder.html#a31a4dba1bcad1b45216ae48b8c124f20
たぶんこっちのCastを使うべき。

In file included from src/mc.cpp:46:
src/codegen.h:92:9: error: cannot jump from switch statement to this case
      label
        default:
        ^
src/codegen.h:90:18: note: jump bypasses variable initialization
            auto cmpvalue = Builder.CreateICmp(CmpInst::ICMP_SLT...
                 ^
これが出たのでautoをやめた。
https://llvm.org/doxygen/classllvm_1_1IntegerType.html

CreateIntCast falseだとzextが出てくる。signedなので直してやる。
https://llvm.org/docs/LangRef.html#zext-to-instruction

```
$ diff -b test/test5_expected_output.txt result/test5
3,4c3,4
<   %ugttmp = icmp ugt i64 %x, 3
<   %cast_i1_to_i64 = sext i1 %ugttmp to i64
---
>   %slttmp = icmp slt i64 %x, 3
>   %cast_i1_to_i64 = sext i1 %slttmp to i64
```
よさそう
1を返すのですが
https://takoeight0821.github.io/posts/2017/09/write-llvm-prog-2.html
参考
a < b -> slt %a %bでよさそう。

// CondVはint64なので、int64の0とequalかどうか判定することでCondVをbool型にする。
    CondV = Builder.CreateICmpEQ(
            CondV, ConstantInt::get(Context, APInt(64, 0)), "ifcond");
0と等しいときにTrueを返すので、Condが0のときにTrueと変換され、Thenが評価される。
これはおかしいのでは

https://github.com/yamaguchi1024/mc-lang-3/blob/master/src/codegen.h#L156
codegenでCondを0と比較するとき、このコードだと"0と等しいときにTrue"と評価されてしまうため、たとえばcondが`0<1`のときは1にキャストされて0と等しくなくfalseに変換されて、ThenではなくElseの方が実行されてしまうように思えます。
正しくはCondが0ではないとき、ではないでしょうか。間違っていたらすみません。

現在、fib(50)くらいをやると再帰で時間がめっちゃかかるので、末尾最適化をしたい。
今のをmc-langでやると、
schemeでは
```
(define (fib n) 
    (define (fib-tail a b count) (
        if (= count 0)
            b
            (fib-tail (+ a b) a (- count 1))
    ))
(
    fib-tail 1 0 n
))
```
になるので、mc-langだと
```
def fibtail(a b c)
  if c == 0 then
    b
  else
    fibtail(a+b a c-1)
def fib(n)
  fibtail(1 0 n)
```

https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl07.html#user-defined-local-variables
User-defined Local Variables
これだ！！！これを参考にやればローカル変数が実装できるのでフィボナッチの末尾最適化ができる。
```
var x = 1, y = 2;
...
```
のように、,区切りで続けて宣言できるらしい
違うっぽい？Kaleidoの仕様がよくわかってないので調べよう。というか自分で決めよう。
;を必ず入れることにする。
Parserは、とりあえず
```
var x = <Expression>;
```
のみを許すようにして、Vecを使ってあとでカンマくぎりに対応するつもり。

思ったけど、関数の引数をExpressionにした方が楽では？改造しよう。
いや普通に動くわ 草

末尾最適化をしない場合fib(45): 4.4s
```
def fib(x)
  # TODO 3.5
  # Delete "1" and write fib here
  if x < 3 then
    1
  else
    fib(x - 1) + fib(x - 2)
```
計測
```
vagrant@vagrant:~/src/mc-lang-3$ time ./main
Call fib with 45: 1134903170

real	0m4.410s
user	0m4.407s
sys	0m0.000s
```
末尾最適化をした場合fib(45): 0.005s
```
def fibtail(a b c)
  if c < 2 then
    b
  else
    fibtail(a+b,a,c-1)
def fib(n)
  fibtail(1,1,n)
```
計測
```
vagrant@vagrant:~/src/mc-lang-3$ time ./main
Call fib with 45: 1134903170

real	0m0.005s
user	0m0.001s
sys	0m0.003s
```

ちょっとびっくりしたので現状報告です。
第3回課題が終了してmc-langで遊んでいたのですが、これ末尾再帰もう書けるんですね...変数定義が必要かなと思ってしばらく変数定義に時間かけていたのですが、ふと何も追加せず書いてみたらあっさり動いてびっくりしました。
以下計測した内容です。まとめると、普通のフィボナッチだと4秒かかる処理が末尾再帰で0.005秒になりました。

末尾再帰をしない場合fib(45): 4.4s
```
def fib(x)
  # TODO 3.5
  # Delete "1" and write fib here
  if x < 3 then
    1
  else
    fib(x - 1) + fib(x - 2)
```
計測
```
vagrant@vagrant:~/src/mc-lang-3$ time ./main
Call fib with 45: 1134903170

real	0m4.410s
user	0m4.407s
sys	0m0.000s
```
末尾再帰をした場合fib(45): 0.005s
```
def fibtail(a b c)
  if c < 2 then
    b
  else
    fibtail(a+b,a,c-1)
def fib(n)
  fibtail(1,1,n)
```
計測
```
vagrant@vagrant:~/src/mc-lang-3$ time ./main
Call fib with 45: 1134903170

real	0m0.005s
user	0m0.001s
sys	0m0.003s
```
