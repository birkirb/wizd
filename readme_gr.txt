***************************************************************************
0.12gr版に関する補足説明
***************************************************************************
これは、wizd改造版(0.12g)に対して、JPEGリサイズ機能を追加したものです。
wizdからimage/jpeg形式のファイルを送信する際に、以下のような変換を行います。
 - リサイズ（ちょうど画面一杯になるように）
 - アスペクト比調整（横長にすることでTVでの表示が縦長になるのを防ぎます）

これらの機能を有効にするには、以下の点に注意してください。
 - Linux（x86およびppc版）でのみコンパイルを確認してます。
 - できればsource/Makefileを使ってください（これでしかテストしてない）
 - コンパイル時に -DRESIZE_JPEG オプションをつける
 - libjpeg6bをインストールしておく(libjpeg-6b-15vl1とlibjpeg-devel-6b-15vl1
   のRPMを入れた環境で開発しました）
 - wizd.conf へ、以下のエントリを追加する（詳細はサンプルを見てください）
   flag_resize_jpeg        true

更新履歴

0.12gr5

　- jpeg_image_viewerから戻る際に、元のカーソル位置に戻るようにしました。

 [更新ファイル]
  + wizd_menu.c
  + jpeg_image_viewer.html (skin)
  + line_image.html (skin)

0.12gr4

  - リサイズ時のメモリ消費を押さえるよう変更

 [更新ファイル]
  + wizd_resize_jpeg.c

0.12gr3

  - png/gifをallplayの対象から外しました。
　- jpeg用のimageviewerスキンとして、jpeg_image_viewer.html を使用する
    ようにしました
  - 349さん作の高画質版wizd_resize_jpeg.cでコンパイルしました

 [更新ファイル]
  + wizd_resize_jpeg.c
  + wizd_menu.c
  + jpeg_image_viewer.html (rename)

0.12gr2

  - 高さ>幅な画像の場合に、変な比率でリサイズしてしまうバグを修正

 [更新ファイル]
  + wizd_resize_jpeg.c

バグなどはMediaWiz系統合スレまで

