from gtts import gTTS
from pydub import AudioSegment

text = "現在購買，第二件十倍，超級加倍中！"
tts = gTTS(text, lang='zh-TW')
tts.save("./ad2.mp3")
AudioSegment.from_mp3("./ad2.mp3").export("./ad2.wav", format="wav")