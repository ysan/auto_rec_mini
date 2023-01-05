import mpegts from 'mpegts.js';

window.addEventListener('load', () => {
  console.log('onload...');

  const button = document.getElementById('button');
  button.addEventListener('click', () => {
    const url = document.getElementById('url');
    play(url.value);
  });
});

const play = (url) => {
  if (mpegts.getFeatureList().mseLivePlayback) {
    const videoElement = document.getElementById('video');
    const player = mpegts.createPlayer({
      type: 'mse',
      isLive: true,
      url: url
    });
    player.attachMediaElement(videoElement);
    player.load();
    player.play();
    player.on('error', (e) => {
      console.log(e);
    });
  }
};
