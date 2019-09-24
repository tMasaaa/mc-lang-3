# 第3回課題と、やろうとしたこと  
  
# 疑問  
疑問1  
> CreateICmpの返り値がi1(1bit)なので、CreateIntCastはそれをint64にcastするのに用います。  
  
ここのi1という根拠をDoxygenの中で探そうとしていたけど分からなかった(Value*以降どこを参照すればよいか進めなかった)  
  
疑問2  
> 3.3 ifのthen,elseで扱える範囲の拡大  
  
僕の実装ではifのthen,elseは必ず一つのExpressionになっています。ここを例えば、変数定義を実装した後  
```  
if a < 1 then  
    b = 1  
    a + b  
else  
    a  
```  
のようにしたい場合、ブロックスコープ`{ Expression1;Expression2;... }`を実装するのが筋がよいのでしょうか？他によい方法はありますか？  
  
## 3.1 '<'を実装してみよう  
まず、演算子の優先順位をどうしようか考えた。ここはC言語(https://ja.cppreference.com/w/c/language/operator_precedence)に倣うと、`<`は`+`や`-`より結合度が低いようなので5に設定した。  
mc.cpp  
```  
    BinopPrecedence['<'] = 10; // like C  
```  
次にcodegen  
CreateICmpは単に比較に関する関数なので、第一引数にどんな比較か指定する必要がある。第4引数はいつものように変数名になる(今回はキャストされる対象にあたる)  
今回は符号付きにしたのでSLTとしている。  
CreateIntCastで第3引数にfalseを指定すると`sext`ではなく`zext`が出てくる。これは何かと思って調べると、 https://llvm.org/docs/LangRef.html#zext-to-instruction より、sextと同じくキャストを行うoperandで、符号付きがsextで符号なし(正確には上位bitの0埋め)がzextである。  
codegen.h  
```  
case '<':  
    L = Builder.CreateICmp(CmpInst::ICMP_SLT, L, R, "slttmp");  
    return Builder.CreateIntCast(L, IntegerType::get(Context, 64), true, "cast_i1_to_i64");  
```  
  
## 3.2 "if", "then", "else"をトークナイズしてみよう  
ここは指示通りに実装する。  
```  
if (identifierStr == "if")  
    return tok_if;  
if (identifierStr == "then")  
    return tok_then;  
if (identifierStr == "else")  
    return tok_else;  
```  
  
## 3.3 If文のパーシングを実装してみよう  
適切にgetNextToken()してif, then, elseを飛ばすのを忘れないように注意して実装する  
  
parser.h  
```  
// return nullptr;  
// TODO 3.3: If文のパーシングを実装してみよう。  
// 1. ParseIfExprに来るということは現在のトークンが"if"なので、  
// トークンを次に進めます。  
getNextToken();  
// 2. ifの次はbranching conditionを表すexpressionがある筈なので、  
// ParseExpressionを呼んでconditionをパースします。  
auto condition = ParseExpression();  
// 3. "if x < 4 then .."のような文の場合、今のトークンは"then"である筈なので  
// それをチェックし、トークンを次に進めます。  
if (CurTok != tok_then)  
    return LogError("Expected `then` in ParseIfExpr()");  
getNextToken(); // skip then  
// 4. "then"ブロックのexpressionをParseExpressionを呼んでパースします。  
auto then_expression = ParseExpression();  
// 5. 3と同様、今のトークンは"else"である筈なのでチェックし、トークンを次に進めます。  
if (CurTok != tok_else)  
    return LogError("Expected `else` in ParseIfExpr");  
getNextToken();  
// 6. "else"ブロックのexpressionをParseExpressionを呼んでパースします。  
auto else_expression = ParseExpression();  
// 7. IfExprASTを作り、returnします。  
return llvm::make_unique<IfExprAST>(std::move(condition), std::move(then_expression), std::move(else_expression));  
```  
  
## 3.4 "else"ブロックのcodegenを実装してみよう  
もともとCreateICmpEQになっていたので、キャスト結果が0になる(condがfalse)のときに、elseが実行されることになっていたのを発見した。(気づけてちょっぴり嬉しい)  
```  
 CondV = Builder.CreateICmpNE(  
    CondV, ConstantInt::get(Context, APInt(64, 0)), "ifcond");  
```  
課題の内容としては、Thenを参考に書けたは書けたが、ブロックという概念がよく分かっていない。  
SSAもよく理解できていなのでもう少し調べたい。  
```  
Builder.SetInsertPoint(ElseBB);  
    Value *ElseV = Else->codegen();  
    if (!ElseV)  
        return nullptr;  
    Builder.CreateBr(MergeBB);  
    ElseBB = Builder.GetInsertBlock();  
    // "ifcont"ブロックのcodegen  
    ParentFunc->getBasicBlockList().push_back(MergeBB);  
    Builder.SetInsertPoint(MergeBB);  
```  
  
## 3.5 n番目のフィボナッチ数列を返す関数をMC言語で書いてみよう  
これは楽しかった。なんと末尾再帰が現段階で動く(変数定義が必要かと思ってめっちゃ時間とかした)  
schemeだと以下のように書ける  
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
よく考えたら、ParseIdentifierExprはArgのParseExpressionをするので、引数に`a+b`みたいなものを入れても壊れない。ここは今まで作ってきたものが自分の理解を超えたみたいな気分で不思議な感じがした。  
mclangでは以下のようになる  
```  
def fibtail(a b c)  
  if c < 2 then  
    b  
  else  
    fibtail(a+b,a,c-1)  
def fib(n)  
  fibtail(1,1,n)  
```  
これが動く！！！すごい！だいたいfib(40)くらいからfib(55)くらいで時間の差が顕著に現れるので、今回はfib(45)で計測。通常の再帰では4s, 末尾再帰は0.005sだった。楽しい。  
  
## やろうとしたこと  
- NOT WORKINGというコミットは現段階での残骸のようなもので、あまり見ないほうがいいコミットです。SSAを念頭に置きながら変数定義を実装しようとするとどういう方針をとればいいのか分からず、Kaleidoscopeを参考にしようとしたがKaleidoscopeも難しくて理解が出来ず断念。