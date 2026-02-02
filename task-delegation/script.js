class WackyRaces {
    constructor() {
        this.participants = [];
        this.isRacing = false;
        this.raceFinished = false;
        this.results = [];
        this.raceStartTime = 0;
        this.timerInterval = null;
        this.animationFrame = null;
        this.weather = 'none';
        
        this.creatures = {
            horses: ['🐴', '🐎', '🏇', '🐴', '🐎', '🏇', '🐴', '🐎', '🏇', '🐴', '🐎', '🏇'],
            unicorns: ['🦄', '🦄', '🦄', '🦄', '🦄', '🦄', '🦄', '🦄', '🦄', '🦄', '🦄', '🦄'],
            dragons: ['🐲', '🐉', '🐲', '🐉', '🔥', '🐲', '🐉', '🐲', '🐉', '🔥', '🐲', '🐉'],
            chickens: ['🐔', '🐓', '🐤', '🐔', '🐓', '🐤', '🐔', '🐓', '🐤', '🐔', '🐓', '🐤'],
            snails: ['🐌', '🐌', '🐌', '🐌', '🐌', '🐌', '🐌', '🐌', '🐌', '🐌', '🐌', '🐌'],
            aliens: ['👽', '🛸', '👾', '🤖', '👽', '🛸', '👾', '🤖', '👽', '🛸', '👾', '🤖'],
            food: ['🍕', '🌮', '🍔', '🍟', '🌭', '🍩', '🧁', '🍿', '🥨', '🍪', '🥐', '🍣'],
            vehicles: ['🚗', '🏎️', '🚕', '🚙', '🛺', '🚜', '🚲', '🛵', '🏍️', '🛴', '🚁', '🚀'],
            chaos: ['🦖', '🦑', '🐙', '🦀', '🐸', '🦆', '🦩', '🐧', '🦭', '🐋', '🦈', '🐊']
        };

        // SauceEngine Team Members
        this.teamNames = [
            'Damon', 'Ziad', 'Raymond', 'Haseeb',
            'Aaron', 'Luis', 'Rowel', 'Faisal',
            'James', 'Kris', 'Maks', 'Nimish'
        ];

        this.commentary = [
            "🎙️ AND THEY'RE OFF!",
            "🎙️ What a tight race!",
            "🎙️ Someone's making a move!",
            "🎙️ This is INCREDIBLE!",
            "🎙️ The crowd goes WILD!",
            "🎙️ History in the making!",
            "🎙️ Neck and neck!",
            "🎙️ Who will take the lead?!",
            "🎙️ ABSOLUTELY ELECTRIC!",
            "🎙️ The tension is PALPABLE!",
            "🎙️ A stunning display of speed!",
            "🎙️ Champions are made on days like this!",
            "🎙️ The lead keeps changing!",
            "🎙️ Nobody's giving up!",
            "🎙️ This could go EITHER WAY!",
            "🎙️ What a BATTLE!",
            "🎙️ They're pushing their limits!",
            "🎙️ Unbelievable determination!",
            "🎙️ The finish line is calling!"
        ];

        // Slower speeds = longer, more dramatic races!
        this.speedConfigs = {
            slow: { min: 0.2, max: 0.8, variance: 0.4 },
            normal: { min: 0.4, max: 1.2, variance: 0.5 },
            fast: { min: 0.8, max: 2.0, variance: 0.6 },
            insane: { min: 1.5, max: 4.0, variance: 0.8 }
        };

        this.colors = [
            '#FF6B6B', '#4ECDC4', '#FFE66D', '#95E1D3',
            '#F38181', '#AA96DA', '#FCBAD3', '#A8D8EA',
            '#FFB6C1', '#98D8C8', '#F7DC6F', '#BB8FCE'
        ];

        this.init();
    }

    init() {
        // Shuffle default team names once per page load for varied starting order
        this.shuffledTeamNames = this.shuffleArray(this.teamNames);
        this.setupEventListeners();
        this.updateParticipantInputs(12); // Full SauceEngine team!
    }

    // Fisher-Yates shuffle for provably fair randomization
    shuffleArray(array) {
        const shuffled = [...array];
        for (let i = shuffled.length - 1; i > 0; i--) {
            const j = Math.floor(Math.random() * (i + 1));
            [shuffled[i], shuffled[j]] = [shuffled[j], shuffled[i]];
        }
        return shuffled;
    }

    // Generate a random seed for transparency
    generateRaceSeed() {
        return Math.random().toString(36).substring(2, 10).toUpperCase();
    }

    setupEventListeners() {
        document.getElementById('numParticipants').addEventListener('change', (e) => {
            const num = Math.min(12, Math.max(2, parseInt(e.target.value) || 4));
            e.target.value = num;
            this.updateParticipantInputs(num);
        });

        document.getElementById('startRaceBtn').addEventListener('click', () => this.startRace());
        document.getElementById('rematchBtn').addEventListener('click', () => this.rematch());
        document.getElementById('newRaceBtn').addEventListener('click', () => this.resetGame());
    }

    updateParticipantInputs(num) {
        const container = document.getElementById('participantsInput');
        container.innerHTML = '';

        const namePool = (this.shuffledTeamNames && this.shuffledTeamNames.length)
            ? this.shuffledTeamNames
            : this.teamNames;

        for (let i = 0; i < num; i++) {
            const card = document.createElement('div');
            card.className = 'participant-card';
            const name = namePool[i] || `Player ${i + 1}`;
            card.innerHTML = `
                <label>🏁 Racer ${i + 1}</label>
                <input type="text" value="${name}" class="participant-name">
                <input type="color" class="participant-color" value="${this.colors[i]}">
            `;
            container.appendChild(card);
        }
    }

    getCreatureEmoji(index, variant) {
        const creatures = this.creatures[variant] || this.creatures.chaos;
        if (variant === 'chaos') {
            // Mix it up for chaos mode
            const allCreatures = Object.values(this.creatures).flat();
            return allCreatures[Math.floor(Math.random() * allCreatures.length)];
        }
        return creatures[index % creatures.length];
    }

    startRace() {
        const numParticipants = parseInt(document.getElementById('numParticipants').value);
        const nameInputs = document.querySelectorAll('.participant-name');
        const colorInputs = document.querySelectorAll('.participant-color');
        const animalVariant = document.getElementById('animalVariant').value;
        const raceSpeed = document.getElementById('raceSpeed').value;
        this.weather = document.getElementById('weatherEffect').value;

        this.participants = [];
        for (let i = 0; i < numParticipants; i++) {
            const name = nameInputs[i].value.trim() || nameInputs[i].placeholder;
            this.participants.push({
                id: i,
                name: name,
                color: colorInputs[i].value,
                emoji: this.getCreatureEmoji(i, animalVariant),
                position: 0,
                speed: 0,
                finished: false,
                finishTime: null,
                lane: i
            });
        }

        this.speedConfig = this.speedConfigs[raceSpeed];
        this.results = [];
        this.isRacing = false;
        this.raceFinished = false;

        // Generate race seed for transparency
        this.raceSeed = this.generateRaceSeed();
        console.log(`🎲 Race Seed: ${this.raceSeed} - All racers have equal probability!`);

        // Preserve the order shown in the setup so lanes match the configured list
        this.participants.forEach((p, i) => {
            p.lane = i;
        });

        // Switch views
        document.getElementById('setupPanel').style.display = 'none';
        document.getElementById('raceContainer').style.display = 'block';
        document.getElementById('resultsPanel').style.display = 'none';

        // Apply weather
        this.applyWeather();

        // Render the race track
        this.renderTrack();

        // Countdown then start
        this.countdown();
    }

    applyWeather() {
        const overlay = document.getElementById('weatherOverlay');
        overlay.className = 'weather-overlay';
        if (this.weather !== 'none') {
            overlay.classList.add(this.weather);
        }
    }

    renderTrack() {
        const container = document.getElementById('lanesContainer');
        container.innerHTML = '';

        const trackWidth = document.getElementById('raceTrack').offsetWidth - 120; // Account for finish line
        this.finishLine = trackWidth;

        this.participants.forEach((p, index) => {
            const lane = document.createElement('div');
            lane.className = 'lane';
            lane.style.borderColor = p.color;
            lane.id = `lane-${p.id}`;

            lane.innerHTML = `
                <div class="lane-number" style="background: ${p.color}; color: white;">${index + 1}</div>
                <div class="racer" id="racer-${p.id}">
                    <span class="racer-emoji">${p.emoji}</span>
                    <span class="racer-name" style="background: ${p.color};">${p.name}</span>
                </div>
            `;

            container.appendChild(lane);
        });
    }

    countdown() {
        const status = document.getElementById('raceStatus');
        const messages = ['🔴 3...', '🟡 2...', '🟢 1...', '🏁 GO GO GO!!!'];
        let i = 0;

        const interval = setInterval(() => {
            status.textContent = messages[i];
            status.style.transform = 'scale(1.2)';
            setTimeout(() => status.style.transform = 'scale(1)', 200);
            
            i++;
            if (i >= messages.length) {
                clearInterval(interval);
                this.beginRace();
            }
        }, 800);
    }

    beginRace() {
        this.isRacing = true;
        this.raceStartTime = Date.now();
        
        // Start timer
        this.timerInterval = setInterval(() => {
            const elapsed = ((Date.now() - this.raceStartTime) / 1000).toFixed(2);
            document.getElementById('raceTimer').textContent = `${elapsed}s`;
        }, 50);

        // Start commentary
        this.startCommentary();

        // Main race loop
        this.raceLoop();
    }

    startCommentary() {
        let lastComment = 0;
        this.commentaryInterval = setInterval(() => {
            if (!this.isRacing) {
                clearInterval(this.commentaryInterval);
                return;
            }
            
            const comment = this.commentary[Math.floor(Math.random() * this.commentary.length)];
            if (comment !== lastComment) {
                document.querySelector('.comment-text').textContent = comment;
                lastComment = comment;
            }
        }, 2000);
    }

    raceLoop() {
        if (!this.isRacing) return;

        let allFinished = true;

        this.participants.forEach(p => {
            if (p.finished) return;
            allFinished = false;

            // Random speed changes for excitement
            if (Math.random() < 0.15) {
                const { min, max, variance } = this.speedConfig;
                const baseSpeed = min + Math.random() * (max - min);
                const weatherModifier = this.getWeatherModifier();
                p.speed = baseSpeed * weatherModifier * (1 + (Math.random() - 0.5) * variance);
            }

            // Occasional boost or slowdown for drama
            if (Math.random() < 0.02) {
                p.speed *= Math.random() < 0.5 ? 1.5 : 0.6;
            }

            p.position += p.speed;

            // Check finish
            if (p.position >= this.finishLine) {
                p.finished = true;
                p.finishTime = Date.now() - this.raceStartTime;
                this.results.push(p);
                
                const racer = document.getElementById(`racer-${p.id}`);
                racer.classList.add('finished');

                // Update status for first finisher
                if (this.results.length === 1) {
                    document.getElementById('raceStatus').textContent = `🏆 ${p.name} WINS!`;
                    document.querySelector('.comment-text').textContent = `🎙️ INCREDIBLE! ${p.name} takes the victory!`;
                }
            }

            // Update position
            const racer = document.getElementById(`racer-${p.id}`);
            if (racer) {
                const displayPos = Math.min(p.position, this.finishLine - 60);
                racer.style.left = `${displayPos}px`;

                // Add dust trail occasionally
                if (Math.random() < 0.1 && p.speed > 3) {
                    this.addDustTrail(racer);
                }
            }
        });

        if (allFinished) {
            this.endRace();
        } else {
            this.animationFrame = requestAnimationFrame(() => this.raceLoop());
        }
    }

    getWeatherModifier() {
        switch (this.weather) {
            case 'rain': return 0.85 + Math.random() * 0.3;
            case 'snow': return 0.7 + Math.random() * 0.4;
            case 'disco': return 0.9 + Math.random() * 0.4;
            default: return 1;
        }
    }

    addDustTrail(racer) {
        const dust = document.createElement('span');
        dust.className = 'dust-trail';
        dust.textContent = '💨';
        racer.appendChild(dust);
        setTimeout(() => dust.remove(), 300);
    }

    endRace() {
        this.isRacing = false;
        this.raceFinished = true;
        
        clearInterval(this.timerInterval);
        clearInterval(this.commentaryInterval);
        cancelAnimationFrame(this.animationFrame);

        document.getElementById('raceStatus').textContent = '🏁 RACE COMPLETE!';

        setTimeout(() => this.showResults(), 1000);
    }

    showResults() {
        document.getElementById('resultsPanel').style.display = 'block';
        
        // Scroll to results
        document.getElementById('resultsPanel').scrollIntoView({ behavior: 'smooth' });

        // Podium
        const positions = ['first', 'second', 'third'];
        positions.forEach((pos, i) => {
            const podium = document.getElementById(`podium${i + 1}`);
            const participant = this.results[i];
            
            if (participant) {
                podium.querySelector('.podium-emoji').textContent = participant.emoji;
                podium.querySelector('.podium-name').textContent = participant.name;
                podium.style.display = 'flex';
            } else {
                podium.style.display = 'none';
            }
        });

        // Full results
        const fullResults = document.getElementById('fullResults');
        fullResults.innerHTML = '';
        
        const medals = ['🥇', '🥈', '🥉', '4️⃣', '5️⃣', '6️⃣', '7️⃣', '8️⃣', '9️⃣', '🔟', '1️⃣1️⃣', '1️⃣2️⃣'];
        
        this.results.forEach((p, i) => {
            const item = document.createElement('div');
            item.className = 'result-item';
            item.innerHTML = `
                <span class="result-position">${medals[i]}</span>
                <span class="result-emoji">${p.emoji}</span>
                <div class="result-info">
                    <div class="result-name">${p.name}</div>
                    <div class="result-time">${(p.finishTime / 1000).toFixed(2)}s</div>
                </div>
            `;
            fullResults.appendChild(item);
        });

        // Confetti!
        this.launchConfetti();
    }

    launchConfetti() {
        const container = document.getElementById('confetti');
        container.innerHTML = '';
        
        const confettiEmojis = ['🎉', '🎊', '✨', '⭐', '🌟', '💫', '🎈', '🎁', '🏆', '👑'];
        
        for (let i = 0; i < 50; i++) {
            setTimeout(() => {
                const confetti = document.createElement('span');
                confetti.className = 'confetti';
                confetti.textContent = confettiEmojis[Math.floor(Math.random() * confettiEmojis.length)];
                confetti.style.left = `${Math.random() * 100}%`;
                confetti.style.animationDuration = `${2 + Math.random() * 2}s`;
                confetti.style.animationDelay = `${Math.random() * 0.5}s`;
                container.appendChild(confetti);
                
                setTimeout(() => confetti.remove(), 4000);
            }, i * 50);
        }
    }

    rematch() {
        // Reset positions but keep same participants
        this.participants.forEach(p => {
            p.position = 0;
            p.speed = 0;
            p.finished = false;
            p.finishTime = null;
        });
        
        this.results = [];
        this.isRacing = false;
        this.raceFinished = false;

        document.getElementById('resultsPanel').style.display = 'none';
        
        this.renderTrack();
        this.countdown();
    }

    resetGame() {
        // Stop everything
        clearInterval(this.timerInterval);
        clearInterval(this.commentaryInterval);
        cancelAnimationFrame(this.animationFrame);
        
        this.participants = [];
        this.results = [];
        this.isRacing = false;
        this.raceFinished = false;

        // Reset views
        document.getElementById('setupPanel').style.display = 'block';
        document.getElementById('raceContainer').style.display = 'none';
        document.getElementById('resultsPanel').style.display = 'none';
        document.getElementById('weatherOverlay').className = 'weather-overlay';
    }
}

// Initialize when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
    window.game = new WackyRaces();
});
