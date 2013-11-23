1.) 死活チェック
rpptで作業
1. crontab -e
2. エディタ起動
3. 下記を追記
* * * * * pgrep "ieee1888_ilonss" || (/etc/init.d/ieee1888_ilonss_gw stop; /etc/init.d/ieee1888_ilonss_gw start)

2.) 自動起動
上記の死活チェックで起動するため、とくべつの設定を行う必要はない
しかし、きちんと起動したい場合には下記のような設定を行う。
1. sudo vi /etc/rc.local
2. 下記を追加
(/etc/init.d/ieee1888_ilonss_gw stop; /etc/init.d/ieee1888_ilonss_gw start)
