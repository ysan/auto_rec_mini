import React, { useState } from 'react';
import LinearProgress from '@mui/material/LinearProgress';

interface Props {
  message: string;
}

export const LinearWaiting: React.FC<Props> = (props: Props) => {
  return (
    <>
      <style>
        {`@keyframes anim_op {
        100% {
          opacity: 0.2;
        }
      }`}
      </style>
      <div
        style={{
          position: 'fixed',
          top: 0,
          left: 0,
          width: '100vw',
          height: '100vh',
          backgroundColor: 'white',
          opacity: 0.8,
          zIndex: 99999
        }}
      >
        <div
          style={{
            position: 'absolute',
            margin: 'auto',
            top: 0,
            bottom: 0,
            left: 0,
            right: 0,
            width: '100%',
            height: '30px',
            textAlign: 'center'
          }}
        >
          <LinearProgress style={{ height: '5px' }} />
          <div
            style={{
              height: '20px',
              opacity: 100,
              animation: `anim_op 0.5s ease 0s infinite alternate`
            }}
          >
            {props.message}
          </div>
        </div>
      </div>
    </>
  );
};

export default LinearWaiting;
