# NCVC ビルド方法

## ディレクトリ構造
以下のようなディレクトリ構造を想定しています．
プロジェクトのパス設定は，ソリューションファイル**NCVC.slh**からの相対指示になっています．
```
.
└── Visual Studio 20XX/
    ├── Projects/
    │   ├── NCVC/
    │   │   ├── NCVC.slh
    │   │   └── NCVC/
    │   ├── Kodatuno/
    │   │   ├── Kodatuno.slh
    │   │   └── Kodatuno.vs/
    │   └── NCVC.Addin/
    │       ├── ReadJW/
    │       ├── ReadCSV/
    │       ├── SendNCD/
    │       ├── SolveTSP/
    │       └── GRBLc/
    ├── include/
    ├── lib/
    │   ├── x86/
    │   └── x64/
    │       └── debug/
    └── boost/
```
もちろんそれぞれの環境に合わせて設定していただいても結構です．
プロジェクトのプロパティから
- C/C++ の 追加のインクルードディレクトリ
- リンカー の 追加のライブラリディレクトリ

をそれぞれ変更してください（Debugモードの Kodatuno.vs.lib だけ stdafx.h 内で設定しています）．  
ただし，この場合はプロジェクト設定(NCVC.vcxproj)をプルリクに含めないようにお願いします．  
ほかに良い設定方法があれば，その旨ご提案ください．

## NCVC.SDK
<https://github.com/NCVC-CAM/NCVC.SDK> のインクルードファイルを適切に設置してください．
lib はアドインのビルドで使いますが，include はNCVC本体のビルドでも使います．

## GLEW
<http://glew.sourceforge.net/> から Windows 32-bit and 64-bit の Binaries をダウンロードしていただき，
インクルードファイルとライブラリファイルを適切に設置してください．

## Boost C++ Library
~~Boost C++ Library をビルドしておいてください．boost::regex を使用しているのでビルドが必要です．~~  
Ver4.14a以降はBoost.Xpressiveに切り替えたので，インクルードファイルだけでOKです．
適切に展開してください．

参考までにビルド用のバッチファイルを以下に示します．

    set BOOST=boost_x_xx_x
    
    cd %BOOST%
    call bootstrap.bat
    bjam toolset=msvc address-model=32 --build-type=complete --stagedir=stage/x86 -j 16
    bjam toolset=msvc address-model=64 --build-type=complete --stagedir=stage/x64 -j 16
    
    cd ..
    rmdir boost
    mklink /D boost %BOOST%

バージョンが変わってもプロジェクトの設定（インクルードファイルやライブラリのパス）を変更しなくて済むように，
シンボリックリンクを作ります．
mklink コマンドだけ管理者権限でないと動かないのでご注意ください．

## Kodatuno
NURBS曲面の切削データ生成等々に，NCVC内部で
金沢大学マンマシン研究室で開発されているKodatunoライブラリを使用しています．
NCVC用にパッチを当てているので，まずこちらのライブラリをご用意ください．
Kodatunoライブラリのビルド方法は <https://github.com/KodatunoOrg/Kodatuno> を参照してください．
NCVCプロジェクトから見えるところに Kodatuno.vs.lib をコピー（またはシンボリックリンク）してもらえればOKです．
インクルードファイルも同様です．僕は下図のようにKodatunoフォルダごとシンボリックリンクを張っています．
NCVC.SDKに含まれるヘッダーも同様にシンボリックリンクしています．  
  
![KodatunoLink.png](./KodatunoLink.png)  
  
NCVC Ver3.91以前はオプションで無効にもできましたが，NCVC Ver4.00以降は Kodatunoライブラリが必須となっています．
