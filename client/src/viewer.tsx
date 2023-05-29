import React, { useState, useEffect, useRef } from 'react';
import { useLocation } from 'react-router-dom';
import { Utils } from './utils';
import Hls from 'hls.js';
import LinearWaiting from './linearWaiting';
import './base.css';
import './viewer.css';

const _dest = process.env.REACT_APP_DESTINATION;

const _state = {
  INIT: 0,
  TUNING: 1,
  TUNE_FAILURE: 2,
  STREAM_WAITING: 3,
  STREAM_FAILURE: 4,
  STREAM_READY: 5
};

interface Props {
  original_network_id: number;
  transport_stream_id: number;
  service_id: number;

  original_network_name: string;
  transport_stream_name: string;
  service_name: string;
}

interface Event {
  original_network_id: number;
  transport_stream_id: number;
  service_id: number;
  event_id: number;

  start_time: number;
  end_time: number;

  event_name_char: string;
}

export const Viewer: React.FC = () => {
  // get props by location
  const location = useLocation();
  const props = location.state as Props;

  const refIsMounted = useRef(true);
  const refUnmountCount = useRef(0);
  const refGroup = useRef(-1);
  const refKeep = useRef<NodeJS.Timer>();
  const refGetEvent = useRef<NodeJS.Timer>();
  const refAbort = useRef<AbortController>();
  const refHls = useRef<Hls>();

  const [state, setState] = useState<number>(_state.INIT);
  const [stateMessage, setStateMessage] = useState<string>('');
  const [presentEvent, setPresentEvent] = useState<Event>();
  const [followEvent, setFollowEvent] = useState<Event>();

  useEffect(() => {
    if (refIsMounted.current) {
      refIsMounted.current = false;
      onMounted();
    }

    return () => {
      refUnmountCount.current++;
      const c = process.env.REACT_APP_BUILD_MODE === 'dev' ? 2 : 1;
      if (refUnmountCount.current === c) {
        onUnmounted();
      }
    };
  }, []);

  const onMounted = async () => {
    console.log('onMounted');
    console.log(props);

    addEventListener('beforeunload', unload, false);
    load();
  };

  const onUnmounted = async () => {
    console.log('onUnmounted');

    unload();
    removeEventListener('beforeunload', unload, false);
  };

  const load = async () => {
    console.log('load');

    refAbort.current?.abort('userabortSafe');

    setState(_state.TUNING);
    setStateMessage('Now tuning...');
    console.log('state', state);

    refAbort.current = new AbortController();
    const groupId = await startViewing(refAbort.current);
    if (groupId >= 0) {
      refGroup.current = groupId;
      refKeep.current = startKeepAliveInterval(groupId, 20, 10);

      await setEvent(groupId);
      refGetEvent.current = startCheckEventInterval(groupId, 10);

      setState(_state.STREAM_WAITING);
      setStateMessage('Waiting for stream...');

      console.log('waiting for stream...');
      const r = await waitForStream(groupId, 3 * 1000, 30);
      if (r === 0) {
        setState(_state.STREAM_READY);
        setStateMessage('Stream is ready');

        refHls.current = new Hls();
        launchHLS(groupId, refHls.current);
      } else {
        console.log('failed to prepare stream');
        await stopViewing(refGroup.current);
        // reset
        refGroup.current = -1;
        stopKeepAliveInterval(refKeep.current);
        stopCheckEventInterval(refGetEvent.current);
        setState(_state.STREAM_FAILURE);
        setStateMessage('Failed to prepare stream');

        if (r === -1) {
          alert('Failed to prepare stream');
          history.back();
        }
      }
    } else {
      console.log('failed to start viewing');
      setState(_state.TUNE_FAILURE);
      setStateMessage('Failed to start viewing');

      console.log('abort reason', refAbort.current?.signal.reason);
      if (
        refAbort.current?.signal.reason !== 'userabort' &&
        refAbort.current?.signal.reason !== 'userabortSafe'
      ) {
        if (refAbort.current?.signal.reason === 'timeout') {
          alert('Failed to start viewing (timeout)');
        } else {
          alert('Failed to start viewing');
        }
        history.back();
      }
    }
  };

  const unload = async () => {
    console.log('unload');
    await stopViewing(refGroup.current);

    // reset
    refGroup.current = -1;
    stopKeepAliveInterval(refKeep.current);
    stopCheckEventInterval(refGetEvent.current);
    setState(_state.INIT);

    refAbort.current?.abort('userabort');
    refHls.current?.destroy();
  };

  const startViewing = async (
    abortController: AbortController
  ): Promise<number> => {
    const body = {
      original_network_id: props.original_network_id,
      transport_stream_id: props.transport_stream_id,
      service_id: props.service_id,
      option: { auto_stop_grace_period_sec: 30 }
    };
    console.log('startViewing request', JSON.stringify(body));

    const response = await Utils.request({
      method: 'POST',
      url: _dest + '/api/ctrl/viewing/start_by_service_id',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(body),
      timeoutMsec: 20 * 1000,
      abortController: abortController
    });
    if (response !== null && response.ok) {
      const res = await response.json();
      console.log('startViewing response', JSON.stringify(res));
      return parseInt(res['group_id']);
    } else {
      return -1;
    }
  };

  const stopViewing = async (groupId: number) => {
    if (groupId < 0) {
      return;
    }

    const body = { group_id: groupId };
    console.log('stopViewing request', JSON.stringify(body));

    const response = await Utils.request({
      method: 'POST',
      url: _dest + '/api/ctrl/viewing/stop',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(body),
      timeoutMsec: 10 * 1000
    });
    console.log('stopViewing response', response);
  };

  const getPresentEvent = async (groupId: number): Promise<string> => {
    const response = await Utils.request({
      method: 'GET',
      url:
        _dest +
        `/api/ctrl/psisi/${groupId}/present_event/${props.original_network_id}/${props.transport_stream_id}/${props.service_id}`,
      timeoutMsec: 2 * 1000
    });
    if (response !== null && response.ok) {
      const j = await response.json();
      return JSON.stringify(j);
    } else {
      return '';
    }
  };

  const getFollowEvent = async (groupId: number): Promise<string> => {
    const response = await Utils.request({
      method: 'GET',
      url:
        _dest +
        `/api/ctrl/psisi/${groupId}/follow_event/${props.original_network_id}/${props.transport_stream_id}/${props.service_id}`,
      timeoutMsec: 5000
    });
    if (response !== null && response.ok) {
      const j = await response.json();
      return JSON.stringify(j);
    } else {
      return '';
    }
  };

  const waitForStream = async (
    groupId: number,
    ticksMsec: number,
    maxCount: number
  ): Promise<number> => {
    for (let i = 0; i < maxCount; i++) {
      //TODO  asynchronous state updates
      //if (state !== _state.stream_waiting) {
      if (refGroup.current === -1) {
        console.log('cancel wait stream');
        return -2;
      }

      await Utils.promisedSleep(ticksMsec);

      const response = await Utils.request({
        method: 'GET',
        url: `${_dest}/stream/${groupId}/stream.m3u8`,
        timeoutMsec: 1 * 1000
      });
      if (response !== null && response.ok) {
        return 0;
      }
    }

    return -1;
  };

  const launchHLS = (groupId: number, hls: Hls) => {
    const video = document.getElementById('video') as HTMLMediaElement;
    const videoSrc = `${_dest}/stream/${groupId}/stream.m3u8`;
    if (Hls.isSupported()) {
      //const hls = new Hls();
      hls.loadSource(videoSrc);
      hls.attachMedia(video as HTMLMediaElement);
      hls.on(Hls.Events.MEDIA_ATTACHED, function () {
        console.log('video and hls.js are now bound together !');
        video.muted = true;
        video.play();
      });
      hls.on(Hls.Events.MANIFEST_PARSED, function (event, data) {
        console.log(
          'manifest loaded, found ' + data.levels.length + ' quality level'
        );
      });
    } else if (video.canPlayType('application/vnd.apple.mpegurl')) {
      video.src = videoSrc;
      video.addEventListener('canplay', () => {
        video.muted = true;
        video.play();
      });
    }
    video.addEventListener('play', () => {
      console.log('onplay');
      setTimeout(() => {
        video.style.objectFit = 'fill'; //TODO for iOS
        video.muted = false;
      }, 500);
    });
    video.addEventListener('playing', () => {
      console.log('onplay');
      setTimeout(() => {
        video.style.objectFit = 'fill'; //TODO for iOS
        video.muted = false;
      }, 500);
    });
  };

  const startKeepAliveInterval = (
    groupId: number,
    gracePeriodSec: number,
    intervalSec: number
  ): NodeJS.Timer => {
    return setInterval(async () => {
      const body = { group_id: groupId, period_sec: gracePeriodSec };
      const response = await Utils.request({
        method: 'POST',
        url: _dest + '/api/ctrl/viewing/auto_stop_grace_period',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(body),
        timeoutMsec: 2 * 1000
      });
      console.log(response);
    }, intervalSec * 1000);
  };

  const stopKeepAliveInterval = (id: NodeJS.Timer | null | undefined) => {
    if (id !== null && id !== undefined) {
      clearInterval(id);
    }
  };

  const setEvent = async (groupId: number) => {
    const pevent = await getPresentEvent(groupId);
    console.log('present event', pevent);
    setPresentEvent(JSON.parse(pevent));

    const fevent = await getFollowEvent(groupId);
    console.log('follow event', fevent);
    setFollowEvent(JSON.parse(fevent));
  };

  const startCheckEventInterval = (
    groupId: number,
    intervalSec: number
  ): NodeJS.Timer => {
    return setInterval(async () => {
      await setEvent(groupId);
    }, intervalSec * 1000);
  };

  const stopCheckEventInterval = (id: NodeJS.Timer | null | undefined) => {
    if (id !== null && id !== undefined) {
      clearInterval(id);
    }
  };

  return (
    <>
      {state !== _state.STREAM_READY && (
        <LinearWaiting message={stateMessage} />
      )}
      <div className="video-outer">
        <div className="video-container">
          <div className="video-area-outer">
            <div className="video-area-inner">
              <video
                id="video"
                autoPlay
                controls
                controlsList="nodownload noremoteplayback"
                disablePictureInPicture
                playsInline
                muted
              />
            </div>
          </div>
          <div className="video-desc">
            <div>
              <span>{props.original_network_name}</span>
              <span>{props.service_name}</span>
              {refGroup.current !== -1 && <span>Tuner:{refGroup.current}</span>}
            </div>

            {presentEvent && (
              <div className="video-desc-present">
                <span>
                  {Utils.getDateTimeString(presentEvent.start_time as number)}
                </span>
                -
                <span>
                  {Utils.getTimeString(presentEvent.end_time as number)}
                </span>
                <span>
                  {(presentEvent.end_time - presentEvent.start_time) / 60}
                  min
                </span>
                <div className="video-desc-title-present">
                  {presentEvent.event_name_char}
                </div>
              </div>
            )}

            {followEvent && (
              <div className="video-desc-follow">
                <span>NEXT:</span>
                <span>
                  {Utils.getDateTimeString(followEvent.start_time as number)}
                </span>
                -
                <span>
                  {Utils.getTimeString(followEvent.end_time as number)}
                </span>
                <span>
                  {(followEvent.end_time - followEvent.start_time) / 60}
                  min
                </span>
                <div className="video-desc-title-follow">
                  {followEvent.event_name_char}
                </div>
              </div>
            )}
          </div>
        </div>
      </div>
    </>
  );
};

export default Viewer;
