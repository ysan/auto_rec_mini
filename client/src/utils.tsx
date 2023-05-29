// eslint-disable-next-line @typescript-eslint/no-namespace
export namespace Utils {
  export interface requestArgs {
    method: string;
    url: string;
    headers?: HeadersInit;
    body?: string;
    timeoutMsec?: number;
    abortController?: AbortController;
  }

  export const request = async (args: requestArgs) => {
    let timer = null;

    let ac: AbortController | null = null;
    if (args.abortController === null || args.abortController === undefined) {
      ac = new AbortController();
    } else {
      ac = args.abortController;
    }

    if (args.timeoutMsec !== null && args.timeoutMsec !== undefined) {
      timer = setTimeout(() => {
        (ac as AbortController).abort('timeout');
      }, args.timeoutMsec);
    }

    try {
      //TODO Cannot find namespace 'window'. (tsserver 2503)
      //const response: window.Error = await fetch(scheme + host + path, {
      const response = await fetch(args.url, {
        method: args.method,
        mode: 'cors',
        ...(args.body !== null &&
          args.body !== undefined && { body: args.body }),
        ...(args.headers !== null &&
          args.headers !== undefined && { headers: args.headers }),
        ...(ac !== null && { signal: ac.signal })
      });

      if (timer !== null) {
        clearTimeout(timer);
      }

      return response;
    } catch (e) {
      //TODO Cannot find namespace 'window'. (tsserver 2503)
      console.log('catched: ' + e);
      if (timer !== null) {
        clearTimeout(timer);
      }

      if (e instanceof window.Error) {
        if (e.name === 'AbortError') {
          console.log('aborted');
        } else {
          console.log('generic error');
        }
      }

      return null;
    }
  };

  export const promisedSleep = async (timeoutMsec: number): Promise<void> => {
    return new Promise((resolve) => {
      setTimeout(() => {
        resolve();
      }, timeoutMsec);
    });
  };

  // YYYY-MM-DD HH:MM:SS
  export function getDateTimeString(sinceEpochTimeSec: number): string {
    const _date = new Date(sinceEpochTimeSec * 1000);
    const r =
      _date.getFullYear() +
      '-' +
      (_date.getMonth() + 1 < 10 ? '0' : '') +
      (_date.getMonth() + 1) +
      '-' +
      (_date.getDate() < 10 ? '0' : '') +
      _date.getDate() +
      ' ' +
      (_date.getHours() < 10 ? '0' : '') +
      _date.getHours() +
      ':' +
      (_date.getMinutes() < 10 ? '0' : '') +
      _date.getMinutes() +
      ':' +
      (_date.getSeconds() < 10 ? '0' : '') +
      _date.getSeconds();
    return r;
  }

  // HH:MM:SS
  export function getTimeString(sinceEpochTimeSec: number): string {
    const _date = new Date(sinceEpochTimeSec * 1000);
    const r =
      (_date.getHours() < 10 ? '0' : '') +
      _date.getHours() +
      ':' +
      (_date.getMinutes() < 10 ? '0' : '') +
      _date.getMinutes() +
      ':' +
      (_date.getSeconds() < 10 ? '0' : '') +
      _date.getSeconds();
    return r;
  }
}
