export class Mutex {
    constructor() {
        this.locked = false;
        this.queue = [];
    }

    async lock() {
        while (this.locked) {
            await new Promise(resolve => this.queue.push(resolve));
        }
        this.locked = true;
    }

    async tryLock() {
        if (this.locked) {
            return false;
        }
        await this.lock();
        return true;
    }

    unlock() {
        this.locked = false;
        if (this.queue.length > 0) {
            const resolve = this.queue.shift();
            resolve();
        }
    }
}