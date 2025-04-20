# Zゲージ向けマスコン

## using devices

* [M5Stack Core2](https://docs.m5stack.com/ja/core/core2)
* [Module USB v1.2](https://docs.m5stack.com/ja/module/USB%20v1.2%20Module)
* [Module 4EncoderMotor](https://docs.m5stack.com/ja/module/Module-4EncoderMotor)
* [ズイキマスコン](https://www.zuiki.co.jp/mascon/zuikiMascon_red_blue/)

## device connection

```
[M5Stach Core2]
  +--- [Module USB v1.2]
  |      +--- [ズイキマスコン]
  |
  +--- [Module 4EncoderMotor]
         +--- (ch1) [Zゲージレールフィーダー]
         +--- (ch3) [Zゲージ電動ポイント(1)]
         +--- (ch4) [Zゲージ電動ポイント(2)]