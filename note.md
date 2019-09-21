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