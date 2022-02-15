# NCVC ビルド方法

## NCVC.SDK
<https://github.com/NCVC-CAM/NCVC.SDK> のインクルードファイルを適切に設置してください．
lib はアドインのビルドで使いますが，include はNCVC本体のビルドでも使います．

## GLEW
<http://glew.sourceforge.net/> から Windows 32-bit and 64-bit の Binaries をダウンロードしていただき，
インクルードファイルとライブラリファイルを適切に設置してください．

## Boost C++ Library
Boost C++ Library をビルドしておいてください．
boost::regex を使用しているのでビルドが必要です．
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
NCコードのシミュレーションにおいて，3Dワークデータと重ねる機能
(WorkFile=hoge.igs)
のIGESファイル読み込みに，金沢大学マンマシン研究室で開発されているKodatunoライブラリを使用しています．
NCVC用にパッチを当てているので，まずこちらのライブラリをご用意ください．
Kodatunoライブラリのビルド方法は <https://github.com/NCVC-CAM/Kodatuno> を参照ください．
NCVCプロジェクトから見えるところに Kodatuno.vs.lib をコピー（またはシンボリックリンク）してもらえればOKです．
インクルードファイルも同様です．僕は下図のようにKodatunoフォルダごとシンボリックリンクを張っています．
NCVC.SDKに含まれるヘッダーも同様にシンボリックリンクしています．
![KodatunoLink.png](./KodatunoLink.png)

それが用意できない場合は stdafx.h の

    #define USE_KODATUNO

をコメントアウトすればKodatunoの機能を無効にした状態でNCVCのビルドは可能です．
今は依存度が低いので #define で対応できますが，今後の予定次第ではKodatunoライブラリが必須になる場合があります．
